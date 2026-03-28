#include <akane/akane.hpp>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "kiseki.hpp"

using json = nlohmann::json;

namespace {

constexpr int kPageSize = 20;

std::string yamlEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out += c;
        }
    }
    return '"' + out + '"';
}

int safeYear(const json& attr) {
    if (!attr.contains("year") || attr["year"].is_null()) return 0;
    if (attr["year"].is_number_integer()) return attr["year"].get<int>();
    return 0;
}

std::string localizedTitle(const json& titleObj) {
    if (!titleObj.is_object()) return "Untitled";
    if (titleObj.contains("en")) return titleObj["en"].get<std::string>();
    for (const auto& [_, v] : titleObj.items()) {
        if (v.is_string()) return v.get<std::string>();
    }
    return "Untitled";
}

std::string idFromRelationship(const json& rels, const std::string& type) {
    if (!rels.is_array()) return "";
    for (const auto& rel : rels) {
        if (rel.value("type", "") == type) return rel.value("id", "");
    }
    return "";
}

std::string extractMeta(const std::string& html, const std::string& key) {
    Kiseki::Parser parser;
    auto root = parser.parse(html);
    for (const auto& node : root->find("meta")) {
        const auto property = node->attr("property");
        const auto name = node->attr("name");
        if (property == key || name == key) {
            return node->attr("content");
        }
    }
    return "";
}

json mangadexGet(const std::string& path, const cpr::Parameters& params = {}) {
    auto response = cpr::Get(
        cpr::Url{"https://api.mangadex.org" + path}, params,
        cpr::Header{{"User-Agent", "mangadawg/2.0"}});
    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error("Mangadex error " + std::to_string(response.status_code));
    }
    return json::parse(response.text);
}

akane::Response ymlResponse(const std::string& content, int status = 200) {
    akane::Response resp(status);
    resp.setHeader("Content-Type", "application/x-yaml; charset=utf-8");
    resp.setBody(content);
    return resp;
}

std::string toYmlList(const json& mangas, int page, const std::string& routeName) {
    std::ostringstream y;
    y << "status: true\n";
    y << "route: " << routeName << "\n";
    y << "page: " << page << "\n";
    y << "page_size: " << kPageSize << "\n";
    y << "results:\n";

    for (const auto& item : mangas) {
        const auto attr = item.value("attributes", json::object());
        const std::string title = localizedTitle(attr.value("title", json::object()));
        const std::string mangaId = item.value("id", "");
        const std::string coverId = idFromRelationship(item.value("relationships", json::array()), "cover_art");

        y << "  - id: " << yamlEscape(mangaId) << "\n";
        y << "    title: " << yamlEscape(title) << "\n";
        y << "    status: " << yamlEscape(attr.value("status", "unknown")) << "\n";
        y << "    year: " << safeYear(attr) << "\n";
        y << "    content_rating: " << yamlEscape(attr.value("contentRating", "safe")) << "\n";
        y << "    latest_chapter: " << yamlEscape(attr.value("latestUploadedChapter", "")) << "\n";
        y << "    cover_id: " << yamlEscape(coverId) << "\n";
    }

    return y.str();
}

cpr::Parameters mangaListParams(int page, const std::string& orderKey) {
    const int offset = (page - 1) * kPageSize;
    return {{"limit", std::to_string(kPageSize)},
            {"offset", std::to_string(offset)},
            {"includes[]", "cover_art"},
            {orderKey, "desc"},
            {"availableTranslatedLanguage[]", "en"}};
}

std::string errorYml(const std::exception& e) { return "status: false\nerror: " + yamlEscape(e.what()) + "\n"; }

}  // namespace

int main() {
    auto server = akane::create_server();

    server.use<akane::CORS>();
    server.use<akane::Compression>();

    server.set_static_directory("public");
    server.enable_static_files(true, "/");

    server.get("/api", [](akane::Context&) -> akane::Response {
        std::ostringstream y;
        y << "name: \"mangadawg api\"\n";
        y << "version: \"2.0\"\n";
        y << "format: \"yml\"\n";
        y << "endpoints:\n";
        y << "  - /api/health\n";
        y << "  - /api/manga/page/:page\n";
        y << "  - /api/manga/popular/:page\n";
        y << "  - /api/recommended\n";
        y << "  - /api/manga/detail/:id\n";
        y << "  - /api/search/:query\n";
        y << "  - /api/chapter/:chapterId\n";
        return ymlResponse(y.str());
    });

    server.get("/api/health", [](akane::Context&) -> akane::Response {
        return ymlResponse("status: true\nservice: \"mangadawg\"\nmessage: \"ok\"\n");
    });

    server.get("/api/manga/page/:page", [](akane::Context& ctx) -> akane::Response {
        try {
            const int page = std::max(1, std::stoi(ctx.req.param("page")));
            auto data = mangadexGet("/manga", mangaListParams(page, "order[latestUploadedChapter]"));
            return ymlResponse(toYmlList(data["data"], page, "latest"));
        } catch (const std::exception& e) {
            return ymlResponse(errorYml(e), 502);
        }
    });

    server.get("/api/manga/popular/:page", [](akane::Context& ctx) -> akane::Response {
        try {
            const int page = std::max(1, std::stoi(ctx.req.param("page")));
            auto data = mangadexGet("/manga", mangaListParams(page, "order[followedCount]"));
            return ymlResponse(toYmlList(data["data"], page, "popular"));
        } catch (const std::exception& e) {
            return ymlResponse(errorYml(e), 502);
        }
    });

    server.get("/api/recommended", [](akane::Context&) -> akane::Response {
        try {
            auto data = mangadexGet(
                "/manga",
                {{"limit", "12"}, {"includes[]", "cover_art"}, {"order[rating]", "desc"},
                 {"availableTranslatedLanguage[]", "en"}, {"hasAvailableChapters", "true"}});
            return ymlResponse(toYmlList(data["data"], 1, "recommended"));
        } catch (const std::exception& e) {
            return ymlResponse(errorYml(e), 502);
        }
    });

    server.get("/api/search/:query", [](akane::Context& ctx) -> akane::Response {
        try {
            const std::string query = ctx.req.param("query");
            auto data = mangadexGet("/manga", {{"limit", "20"},
                                                {"title", query},
                                                {"includes[]", "cover_art"},
                                                {"availableTranslatedLanguage[]", "en"}});
            std::string yml = toYmlList(data["data"], 1, "search");
            yml += "query: " + yamlEscape(query) + "\n";
            return ymlResponse(yml);
        } catch (const std::exception& e) {
            return ymlResponse(errorYml(e), 502);
        }
    });

    server.get("/api/manga/detail/:id", [](akane::Context& ctx) -> akane::Response {
        try {
            const std::string id = ctx.req.param("id");
            auto detail = mangadexGet("/manga/" + id, {{"includes[]", "cover_art"}, {"includes[]", "author"}});
            auto chapters = mangadexGet(
                "/manga/" + id + "/feed",
                {{"limit", "60"}, {"order[chapter]", "asc"}, {"translatedLanguage[]", "en"}});

            const auto data = detail["data"];
            const auto attr = data["attributes"];
            std::string description = attr.value("description", json::object()).value("en", "No description");

            std::string ogImage;
            auto pageResponse = cpr::Get(cpr::Url{"https://mangadex.org/title/" + id},
                                         cpr::Header{{"User-Agent", "mangadawg/2.0"}});
            if (pageResponse.status_code >= 200 && pageResponse.status_code < 300) {
                ogImage = extractMeta(pageResponse.text, "og:image");
                const auto ogDesc = extractMeta(pageResponse.text, "og:description");
                if (!ogDesc.empty()) description = ogDesc;
            }

            std::ostringstream y;
            y << "status: true\n";
            y << "id: " << yamlEscape(id) << "\n";
            y << "title: " << yamlEscape(localizedTitle(attr.value("title", json::object()))) << "\n";
            y << "description: " << yamlEscape(description) << "\n";
            y << "state: " << yamlEscape(attr.value("status", "unknown")) << "\n";
            y << "cover_from_page: " << yamlEscape(ogImage) << "\n";
            y << "genres:\n";
            for (const auto& tag : attr.value("tags", json::array())) {
                const auto name = tag.value("attributes", json::object()).value("name", json::object()).value("en", "");
                if (!name.empty()) y << "  - " << yamlEscape(name) << "\n";
            }
            y << "chapters:\n";
            for (const auto& ch : chapters["data"]) {
                const auto cAttr = ch.value("attributes", json::object());
                y << "  - id: " << yamlEscape(ch.value("id", "")) << "\n";
                y << "    chapter: " << yamlEscape(cAttr.value("chapter", "?")) << "\n";
                y << "    title: " << yamlEscape(cAttr.value("title", "")) << "\n";
                y << "    pages: " << cAttr.value("pages", 0) << "\n";
            }
            return ymlResponse(y.str());
        } catch (const std::exception& e) {
            return ymlResponse(errorYml(e), 502);
        }
    });

    server.get("/api/chapter/:chapterId", [](akane::Context& ctx) -> akane::Response {
        try {
            const std::string chapterId = ctx.req.param("chapterId");
            auto home = mangadexGet("/at-home/server/" + chapterId);
            const auto chapter = home.value("chapter", json::object());

            std::ostringstream y;
            y << "status: true\n";
            y << "chapter_id: " << yamlEscape(chapterId) << "\n";
            y << "pages:\n";

            const std::string baseUrl = home.value("baseUrl", "");
            const std::string hash = chapter.value("hash", "");
            auto pages = chapter.value("data", json::array());
            bool dataSaver = false;
            if (pages.empty()) {
                pages = chapter.value("dataSaver", json::array());
                dataSaver = true;
            }

            const std::string pathPrefix = dataSaver ? "/data-saver/" : "/data/";
            for (const auto& page : pages) {
                y << "  - " << yamlEscape(baseUrl + pathPrefix + hash + "/" + page.get<std::string>()) << "\n";
            }
            return ymlResponse(y.str());
        } catch (const std::exception& e) {
            return ymlResponse(errorYml(e), 502);
        }
    });

    server.get("/", [](akane::Context&) -> akane::Response {
        return akane::Response(200).file("public/index.html");
    });

    constexpr int kPort = 8080;
    server.bind("0.0.0.0", kPort);
    std::cout << "Mangadawg running at http://127.0.0.1:" << kPort << "\n";
    server.start();
    return 0;
}
