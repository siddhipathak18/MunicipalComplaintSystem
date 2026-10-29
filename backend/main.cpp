#include "httplib.h"
#include "complaint.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>

// Mutex to prevent concurrent file access issues
std::mutex fileMutex;

// ============================================================
// SIMPLE JSON HELPERS (no external library needed)
// ============================================================

// Escape a string for JSON output
std::string jsonEscape(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c;
        }
    }
    return result;
}

// Convert a Complaint to a JSON string
std::string complaintToJson(const Complaint& c) {
    std::ostringstream json;
    json << "{";
    json << "\"id\":" << c.id << ",";
    json << "\"complaintId\":\"CMP" << c.id << "\",";
    json << "\"name\":\"" << jsonEscape(c.name) << "\",";
    json << "\"phone\":\"" << jsonEscape(c.phone) << "\",";
    json << "\"category\":\"" << jsonEscape(c.category) << "\",";
    json << "\"description\":\"" << jsonEscape(c.description) << "\",";
    json << "\"location\":\"" << jsonEscape(c.location) << "\",";
    json << "\"priority\":\"" << jsonEscape(c.priority) << "\",";
    json << "\"dateTime\":\"" << jsonEscape(c.dateTime) << "\",";
    json << "\"status\":\"" << jsonEscape(c.status) << "\"";
    json << "}";
    return json.str();
}

// Read the entire contents of a file into a string
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Extract a JSON string value by key (simple parser for flat JSON)
std::string getJsonValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;

    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        // String value
        pos++;
        std::string value;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                switch (json[pos]) {
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    default: value += json[pos];
                }
            } else {
                value += json[pos];
            }
            pos++;
        }
        return value;
    }
    return "";
}

// Determine the MIME type based on file extension
std::string getMimeType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos)  return "text/css";
    if (path.find(".js") != std::string::npos)   return "application/javascript";
    return "text/plain";
}

// ============================================================
// MAIN SERVER
// ============================================================

int main() {
    httplib::Server server;

    std::cout << "===================================" << std::endl;
    std::cout << "  Municipal Complaint System" << std::endl;
    std::cout << "  Server starting..." << std::endl;
    std::cout << "===================================" << std::endl;

    // ----------------------------------------------------------
    // SERVE STATIC FILES (Frontend)
    // ----------------------------------------------------------

    // Serve citizen homepage
    server.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string content = readFile("frontend/index.html");
        if (content.empty()) {
            res.status = 404;
            res.set_content("File not found", "text/plain");
            return;
        }
        res.set_content(content, "text/html");
    });

    // Serve admin page
    server.Get("/admin", [](const httplib::Request&, httplib::Response& res) {
        std::string content = readFile("frontend/admin.html");
        if (content.empty()) {
            res.status = 404;
            res.set_content("File not found", "text/plain");
            return;
        }
        res.set_content(content, "text/html");
    });

    // Serve CSS and JS files
    server.Get("/style.css", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(readFile("frontend/style.css"), "text/css");
    });

    server.Get("/script.js", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(readFile("frontend/script.js"), "application/javascript");
    });

    // ----------------------------------------------------------
    // API ENDPOINTS
    // ----------------------------------------------------------

    // POST /api/complaints - Submit a new complaint
    server.Post("/api/complaints", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(fileMutex);

        std::string body = req.body;

        // Extract fields from JSON body
        std::string name        = getJsonValue(body, "name");
        std::string phone       = getJsonValue(body, "phone");
        std::string category    = getJsonValue(body, "category");
        std::string description = getJsonValue(body, "description");
        std::string location    = getJsonValue(body, "location");
        std::string priority    = getJsonValue(body, "priority");

        // Validate priority (default to Medium if empty or invalid)
        if (priority != "Low" && priority != "Medium" && priority != "High") {
            priority = "Medium";
        }

        // Validate required fields
        if (name.empty() || phone.empty() || category.empty() ||
            description.empty() || location.empty()) {
            res.status = 400;
            res.set_content(
                "{\"success\":false,\"message\":\"All fields are required\"}",
                "application/json");
            return;
        }

        // Create the complaint
        Complaint complaint;
        complaint.id          = generateNextId();
        complaint.name        = name;
        complaint.phone       = phone;
        complaint.category    = category;
        complaint.description = description;
        complaint.location    = location;
        complaint.priority    = priority;
        complaint.dateTime    = getCurrentDateTime();
        complaint.status      = "Pending";

        // Save to CSV
        if (saveComplaint(complaint)) {
            std::ostringstream json;
            json << "{\"success\":true,";
            json << "\"complaintId\":\"CMP" << complaint.id << "\",";
            json << "\"message\":\"Complaint submitted successfully\"}";
            res.set_content(json.str(), "application/json");
            std::cout << "[NEW] Complaint CMP" << complaint.id << " from " << name << std::endl;
        } else {
            res.status = 500;
            res.set_content(
                "{\"success\":false,\"message\":\"Failed to save complaint\"}",
                "application/json");
        }
    });

    // GET /api/complaints - Get all complaints
    server.Get("/api/complaints", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(fileMutex);

        std::vector<Complaint> complaints = loadComplaints();
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < complaints.size(); i++) {
            if (i > 0) json << ",";
            json << complaintToJson(complaints[i]);
        }
        json << "]";
        res.set_content(json.str(), "application/json");
    });

    // GET /api/complaints/:id - Get a specific complaint
    server.Get(R"(/api/complaints/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(fileMutex);

        int id;
        try {
            id = std::stoi(req.matches[1]);
        } catch (...) {
            res.status = 400;
            res.set_content(
                "{\"success\":false,\"message\":\"Invalid complaint ID\"}",
                "application/json");
            return;
        }

        Complaint c = findComplaint(id);
        if (c.id == -1) {
            res.status = 404;
            res.set_content(
                "{\"success\":false,\"message\":\"Complaint not found\"}",
                "application/json");
            return;
        }

        res.set_content(complaintToJson(c), "application/json");
    });

    // PUT /api/complaints/:id/status - Update complaint status
    server.Put(R"(/api/complaints/(\d+)/status)", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(fileMutex);

        int id;
        try {
            id = std::stoi(req.matches[1]);
        } catch (...) {
            res.status = 400;
            res.set_content(
                "{\"success\":false,\"message\":\"Invalid complaint ID\"}",
                "application/json");
            return;
        }

        std::string newStatus = getJsonValue(req.body, "status");
        if (newStatus.empty()) {
            res.status = 400;
            res.set_content(
                "{\"success\":false,\"message\":\"Status is required\"}",
                "application/json");
            return;
        }

        if (newStatus != "Pending" && newStatus != "In Progress" && newStatus != "Resolved") {
            res.status = 400;
            res.set_content(
                "{\"success\":false,\"message\":\"Invalid status. Use: Pending, In Progress, or Resolved\"}",
                "application/json");
            return;
        }

        if (updateComplaintStatus(id, newStatus)) {
            std::ostringstream json;
            json << "{\"success\":true,\"message\":\"Status updated to " << jsonEscape(newStatus) << "\"}";
            res.set_content(json.str(), "application/json");
            std::cout << "[UPDATE] CMP" << id << " -> " << newStatus << std::endl;
        } else {
            res.status = 404;
            res.set_content(
                "{\"success\":false,\"message\":\"Complaint not found or update failed\"}",
                "application/json");
        }
    });

    // ----------------------------------------------------------
    // START SERVER
    // ----------------------------------------------------------
    std::cout << std::endl;
    std::cout << "Server is running at: http://localhost:8081" << std::endl;
    std::cout << "Citizen Portal:       http://localhost:8081" << std::endl;
    std::cout << "Admin Dashboard:      http://localhost:8081/admin" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop the server." << std::endl;
    std::cout << "===================================" << std::endl;

    server.listen("0.0.0.0", 8081);

    return 0;
}
