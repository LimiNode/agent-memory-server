#include <agent_memory/infrastructure/mdbx/MdbxResourceImporter.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

using agent_memory::MdbxResourceImporter;
using agent_memory::MdbxResourceImporterOptions;
using agent_memory::ObservedResource;
using agent_memory::ResourceId;
using agent_memory::ResourceImportPublicationProfile;
using agent_memory::ScopeId;

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
    const auto path = std::filesystem::temp_directory_path() /
                      "agent-memory-server-vs01-core-smoke";
    std::error_code cleanup_error;
    std::filesystem::remove_all(path, cleanup_error);

    try {
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

    std::filesystem::remove_all(path, cleanup_error);
    return 0;
}
