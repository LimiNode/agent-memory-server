#include <agent_memory/infrastructure/mdbx/MdbxResourceImporter.hpp>

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {

using agent_memory::MdbxResourceImporter;
using agent_memory::MdbxResourceImporterOptions;
using agent_memory::ObservedResource;
using agent_memory::ResourceId;
using agent_memory::ResourceImportPublicationProfile;
using agent_memory::ScopeId;

class OwnedTempDirectory final {
public:
    explicit OwnedTempDirectory(std::filesystem::path path)
        : m_path(std::move(path)) {}

    OwnedTempDirectory(const OwnedTempDirectory&) = delete;
    OwnedTempDirectory& operator=(const OwnedTempDirectory&) = delete;

    OwnedTempDirectory(OwnedTempDirectory&& other) noexcept
        : m_path(std::move(other.m_path)), m_owned(other.m_owned) {
        other.m_owned = false;
    }

    OwnedTempDirectory& operator=(OwnedTempDirectory&& other) noexcept {
        if(this != &other) {
            cleanup();
            m_path = std::move(other.m_path);
            m_owned = other.m_owned;
            other.m_owned = false;
        }
        return *this;
    }

    ~OwnedTempDirectory() { (void)cleanup(); }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return m_path;
    }

    bool cleanup() noexcept {
        if(!m_owned) {
            return true;
        }
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
        m_owned = false;
        if(error) {
            std::cerr << "temporary directory cleanup failed for "
                      << m_path << ": " << error.message() << '\n';
            return false;
        }
        return true;
    }

private:
    std::filesystem::path m_path;
    bool m_owned = true;
};

std::optional<OwnedTempDirectory> try_reserve_directory(
    const std::filesystem::path& candidate,
    std::error_code& error
) {
    error.clear();
    if(std::filesystem::create_directory(candidate, error)) {
        return OwnedTempDirectory{candidate};
    }
    if(!error) {
        error = std::make_error_code(std::errc::file_exists);
    }
    return std::nullopt;
}

OwnedTempDirectory make_unique_temp_directory() {
    std::error_code error;
    const auto base = std::filesystem::temp_directory_path(error);
    if(error) {
        throw std::runtime_error("cannot resolve temporary directory: " + error.message());
    }

    std::random_device random;
    std::mt19937_64 generator(
        (static_cast<std::uint64_t>(random()) << 32U) ^ random()
    );
    for(std::size_t attempt = 0; attempt < 128; ++attempt) {
        const auto candidate = base /
            ("agent-memory-server-vs01-" + std::to_string(generator()));
        std::error_code reserve_error;
        if(auto owned = try_reserve_directory(candidate, reserve_error)) {
            return std::move(*owned);
        }
        std::error_code exists_error;
        if(std::filesystem::exists(candidate, exists_error)) {
            continue;
        }
        if(exists_error) {
            throw std::runtime_error(
                "cannot inspect temporary directory candidate: " +
                exists_error.message());
        }
        throw std::runtime_error(
            "cannot reserve temporary directory: " + reserve_error.message());
    }
    throw std::runtime_error("could not reserve a unique temporary directory");
}

bool verify_existing_directory_is_not_claimed(const std::filesystem::path& base) {
    const auto candidate = base / "agent-memory-server-vs01-existing";
    std::error_code error;
    if(!std::filesystem::create_directory(candidate, error) || error) {
        std::cerr << "cannot prepare collision regression directory: "
                  << error.message() << '\n';
        return false;
    }
    OwnedTempDirectory existing{candidate};
    const auto sentinel = candidate / "sentinel.txt";
    {
        std::ofstream output(sentinel, std::ios::binary);
        if(!output) {
            std::cerr << "cannot create collision sentinel\n";
            return false;
        }
        output << "must survive";
        output.flush();
        if(!output) {
            std::cerr << "cannot write collision sentinel\n";
            return false;
        }
    }

    std::error_code reserve_error;
    if(try_reserve_directory(candidate, reserve_error).has_value() ||
       !reserve_error) {
        std::cerr << "existing directory was incorrectly claimed\n";
        return false;
    }
    std::ifstream input(sentinel, std::ios::binary);
    if(!input) {
        std::cerr << "cannot read collision sentinel\n";
        return false;
    }
    std::string contents;
    std::getline(input, contents);
    input.close();
    if(contents != "must survive") {
        std::cerr << "existing directory sentinel changed\n";
        return false;
    }
    return existing.cleanup();
}

MdbxResourceImporterOptions options(const std::filesystem::path& path) {
    MdbxResourceImporterOptions value;
    value.path = path.string();
    value.publication_profile = ResourceImportPublicationProfile::LexicalOnly;
    return value;
}

ObservedResource resource(std::string body) {
    ObservedResource value;
    value.scope_id = ScopeId{"vs01-scope"};
    value.resource_id = ResourceId{"vs01-resource"};
    value.source_uri = "memory://vs01-resource";
    value.utf8_body = std::move(body);
    return value;
}

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

} // namespace

int main() {
    std::optional<OwnedTempDirectory> temporary;
    try {
        temporary = make_unique_temp_directory();
        if(!verify_existing_directory_is_not_claimed(temporary->path())) {
            return fail("temporary directory ownership regression failed");
        }
        const auto& path = temporary->path();
        {
            MdbxResourceImporter importer{options(path)};
            const auto result = importer.import(resource("first durable decision"));
            if(result.revision.generation != 1) {
                return fail("initial generation mismatch");
            }
            const auto hits = importer.search(ScopeId{"vs01-scope"}, "durable decision", 5);
            if(hits.size() != 1 || hits.front().generation != 1) {
                return fail("initial lexical search mismatch");
            }
        }

        {
            MdbxResourceImporter reopened{options(path)};
            const auto hits = reopened.search(ScopeId{"vs01-scope"}, "durable decision", 5);
            if(hits.size() != 1 || hits.front().resource_id != ResourceId{"vs01-resource"}) {
                return fail("reopen search mismatch");
            }
            const auto result = reopened.import(resource("second durable decision"));
            if(result.revision.generation != 2) {
                return fail("updated generation mismatch");
            }
            if(!reopened.search(ScopeId{"vs01-scope"}, "first", 5).empty()) {
                return fail("stale lexical term remained after update");
            }
            if(reopened.search(ScopeId{"vs01-scope"}, "second", 5).size() != 1) {
                return fail("updated lexical term missing");
            }
        }
    } catch(const std::exception& error) {
        std::cerr << "smoke test exception: " << error.what() << '\n';
        return 1;
    }
    return temporary->cleanup() ? 0 : 1;
}
