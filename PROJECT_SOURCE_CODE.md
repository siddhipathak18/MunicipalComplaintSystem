# Project Source Code

Project:
Complaint Management System for Municipal Corporation

Technology:
C++17 + cpp-httplib + HTML5 + CSS3 + Vanilla JavaScript + CSV

Server:
http://localhost:8081

Citizen Portal:
http://localhost:8081

Admin Portal:
http://localhost:8081/admin

---
## backend/main.cpp

```cpp
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

    server.listen("0.0.0.0", 8080);

    return 0;
}
```

---

## backend/complaint.h

```cpp
#ifndef COMPLAINT_H
#define COMPLAINT_H

#include <string>
#include <vector>

// Structure to represent a single complaint
struct Complaint {
    int id;                  // Numeric ID (e.g., 1001)
    std::string name;        // Citizen's full name
    std::string phone;       // Phone number
    std::string category;    // Complaint category
    std::string description; // Detailed description
    std::string location;    // Location of the issue
    std::string priority;    // Priority: Low, Medium, High
    std::string dateTime;    // Submission timestamp: DD-MM-YYYY HH:MM
    std::string status;      // Status: Pending, In Progress, Resolved
};

// CSV file path (relative to project root)
const std::string CSV_FILE = "data/complaints.csv";

// Load all complaints from the CSV file
std::vector<Complaint> loadComplaints();

// Save a new complaint to the CSV file
bool saveComplaint(const Complaint& complaint);

// Find a complaint by its numeric ID
Complaint findComplaint(int id);

// Update the status of a complaint
bool updateComplaintStatus(int id, const std::string& newStatus);

// Generate the next complaint ID
int generateNextId();

// Helper: Get current system date and time in DD-MM-YYYY HH:MM format
std::string getCurrentDateTime();

// Helper: Escape a string for CSV (handle commas, quotes, newlines)
std::string escapeCSV(const std::string& field);

// Helper: Parse a single CSV line into fields
std::vector<std::string> parseCSVLine(const std::string& line);

// Helper: Remove surrounding quotes from a CSV field
std::string unescapeCSV(const std::string& field);

#endif
```

---

## backend/complaint.cpp

```cpp
#include "complaint.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <ctime>

// ============================================================
// CSV HELPER FUNCTIONS
// ============================================================

// Helper: Get current system date and time in DD-MM-YYYY HH:MM format
std::string getCurrentDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm* ltm = std::localtime(&now);
    char buffer[32];
    if (ltm) {
        std::strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M", ltm);
        return std::string(buffer);
    }
    return "01-01-2026 00:00";
}

// Escape a field for CSV: wrap in quotes if it contains comma, quote, or newline
std::string escapeCSV(const std::string& field) {
    bool needsQuoting = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needsQuoting = true;
            break;
        }
    }
    if (!needsQuoting) return field;

    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') escaped += "\"\"";
        else escaped += c;
    }
    escaped += '"';
    return escaped;
}

// Remove surrounding quotes and unescape doubled quotes
std::string unescapeCSV(const std::string& field) {
    if (field.size() >= 2 && field.front() == '"' && field.back() == '"') {
        std::string inner = field.substr(1, field.size() - 2);
        std::string result;
        for (size_t i = 0; i < inner.size(); i++) {
            if (inner[i] == '"' && i + 1 < inner.size() && inner[i + 1] == '"') {
                result += '"';
                i++; // skip the second quote
            } else {
                result += inner[i];
            }
        }
        return result;
    }
    return field;
}

// Parse a CSV line respecting quoted fields
std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    i++;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else if (c == '\r') {
                // skip carriage return
            } else {
                current += c;
            }
        }
    }
    fields.push_back(current);
    return fields;
}

// ============================================================
// COMPLAINT FUNCTIONS
// ============================================================

// Ensure the CSV file exists with a header row
static void ensureCSVExists() {
    std::ifstream check(CSV_FILE);
    if (!check.good()) {
        std::ofstream create(CSV_FILE);
        create << "id,name,phone,category,description,location,priority,date_time,status" << std::endl;
        create.close();
    }
    check.close();
}

// Load all complaints from the CSV file
std::vector<Complaint> loadComplaints() {
    ensureCSVExists();
    std::vector<Complaint> complaints;
    std::ifstream file(CSV_FILE);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << CSV_FILE << std::endl;
        return complaints;
    }

    std::string line;
    // Skip the header line
    std::getline(file, line);

    // Read each complaint line
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<std::string> fields = parseCSVLine(line);
        if (fields.size() >= 9) {
            // New 9-column format
            Complaint c;
            try {
                c.id = std::stoi(fields[0]);
            } catch (...) {
                continue; // Skip malformed lines
            }
            c.name        = fields[1];
            c.phone       = fields[2];
            c.category    = fields[3];
            c.description = fields[4];
            c.location    = fields[5];
            c.priority    = fields[6].empty() ? "Medium" : fields[6];
            c.dateTime    = fields[7].empty() ? "Not Available" : fields[7];
            c.status      = fields[8];
            complaints.push_back(c);
        } else if (fields.size() >= 7) {
            // Backward compatibility for older 7-column CSV records
            Complaint c;
            try {
                c.id = std::stoi(fields[0]);
            } catch (...) {
                continue; // Skip malformed lines
            }
            c.name        = fields[1];
            c.phone       = fields[2];
            c.category    = fields[3];
            c.description = fields[4];
            c.location    = fields[5];
            c.priority    = "Medium";
            c.dateTime    = "Not Available";
            c.status      = fields[6];
            complaints.push_back(c);
        }
    }

    file.close();
    return complaints;
}

// Save a new complaint (append to CSV)
bool saveComplaint(const Complaint& complaint) {
    ensureCSVExists();
    std::ofstream file(CSV_FILE, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << CSV_FILE << " for writing" << std::endl;
        return false;
    }

    // Write complaint as a 9-column CSV row
    file << complaint.id << ","
         << escapeCSV(complaint.name) << ","
         << escapeCSV(complaint.phone) << ","
         << escapeCSV(complaint.category) << ","
         << escapeCSV(complaint.description) << ","
         << escapeCSV(complaint.location) << ","
         << escapeCSV(complaint.priority) << ","
         << escapeCSV(complaint.dateTime) << ","
         << escapeCSV(complaint.status) << std::endl;

    file.close();
    return true;
}

// Find a complaint by its numeric ID
Complaint findComplaint(int id) {
    std::vector<Complaint> complaints = loadComplaints();
    for (const auto& c : complaints) {
        if (c.id == id) return c;
    }
    // Return an empty complaint with id = -1 if not found
    Complaint notFound;
    notFound.id = -1;
    return notFound;
}

// Update the status of a complaint (rewrites the entire CSV)
bool updateComplaintStatus(int id, const std::string& newStatus) {
    // Validate the new status
    if (newStatus != "Pending" && newStatus != "In Progress" && newStatus != "Resolved") {
        return false;
    }

    std::vector<Complaint> complaints = loadComplaints();
    bool found = false;

    // Find and update the complaint
    for (auto& c : complaints) {
        if (c.id == id) {
            c.status = newStatus;
            found = true;
            break;
        }
    }

    if (!found) return false;

    // Rewrite the entire CSV file with the updated 9-column format
    std::ofstream file(CSV_FILE, std::ios::trunc);
    if (!file.is_open()) return false;

    file << "id,name,phone,category,description,location,priority,date_time,status" << std::endl;
    for (const auto& c : complaints) {
        file << c.id << ","
             << escapeCSV(c.name) << ","
             << escapeCSV(c.phone) << ","
             << escapeCSV(c.category) << ","
             << escapeCSV(c.description) << ","
             << escapeCSV(c.location) << ","
             << escapeCSV(c.priority) << ","
             << escapeCSV(c.dateTime) << ","
             << escapeCSV(c.status) << std::endl;
    }

    file.close();
    return true;
}

// Generate the next complaint ID (starts from 1001)
int generateNextId() {
    std::vector<Complaint> complaints = loadComplaints();
    int maxId = 1000; // IDs start from 1001
    for (const auto& c : complaints) {
        if (c.id > maxId) maxId = c.id;
    }
    return maxId + 1;
}
```

---

## frontend/index.html

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Municipal Complaint Management System</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <!-- Navigation Bar -->
    <nav class="navbar">
        <div class="nav-brand">
            <h1>🏛️ Municipal Corporation</h1>
            <p>Complaint Management System</p>
        </div>
        <div class="nav-links">
            <a href="#" onclick="showSection('home')" class="nav-link active">Home</a>
            <a href="#" onclick="showSection('submit')" class="nav-link">Submit Complaint</a>
            <a href="#" onclick="showSection('track')" class="nav-link">Track Complaint</a>
            <a href="/admin" class="nav-link">Admin</a>
        </div>
    </nav>

    <!-- HOME SECTION -->
    <div id="home" class="section active">
        <div class="hero">
            <h2>Report. Track. Resolve.</h2>
            <p>Submit civic complaints and track their resolution status easily.</p>
            <button onclick="showSection('submit')" class="hero-btn">Submit a Complaint</button>
        </div>
        <!-- Quick info cards -->
        <div class="container">
            <div class="info-cards">
                <div class="info-card">
                    <div class="info-icon">📝</div>
                    <h3>Submit</h3>
                    <p>File your civic complaint with details about the issue.</p>
                </div>
                <div class="info-card">
                    <div class="info-icon">🔍</div>
                    <h3>Track</h3>
                    <p>Use your Complaint ID to check real-time status updates.</p>
                </div>
                <div class="info-card">
                    <div class="info-icon">✅</div>
                    <h3>Resolve</h3>
                    <p>Municipal officers review and resolve your complaints.</p>
                </div>
            </div>
        </div>
    </div>

    <!-- SUBMIT COMPLAINT SECTION -->
    <div id="submit" class="section">
        <div class="container">
            <div class="form-card">
                <h2>📝 Submit a Complaint</h2>
                <p class="form-subtitle">Fill in the details below to register your complaint.</p>

                <form id="complaintForm" onsubmit="submitComplaint(event)">
                    <div class="form-group">
                        <label for="name">Citizen Name *</label>
                        <input type="text" id="name" placeholder="Enter your full name" required>
                    </div>

                    <div class="form-group">
                        <label for="phone">Phone Number *</label>
                        <input type="text" id="phone" placeholder="Enter your phone number" required>
                    </div>

                    <div class="form-group">
                        <label for="category">Complaint Category *</label>
                        <select id="category" required>
                            <option value="">-- Select Category --</option>
                            <option value="Road Damage">Road Damage</option>
                            <option value="Garbage">Garbage</option>
                            <option value="Water Supply">Water Supply</option>
                            <option value="Street Light">Street Light</option>
                            <option value="Drainage">Drainage</option>
                            <option value="Other">Other</option>
                        </select>
                    </div>

                    <div class="form-group">
                        <label for="description">Description *</label>
                        <textarea id="description" rows="4" placeholder="Describe your complaint in detail" required></textarea>
                    </div>

                    <div class="form-group">
                        <label for="location">Location *</label>
                        <input type="text" id="location" placeholder="Enter the location of the issue" required>
                    </div>

                    <div class="form-group">
                        <label for="priority">Priority *</label>
                        <select id="priority" required>
                            <option value="Low">Low</option>
                            <option value="Medium" selected>Medium</option>
                            <option value="High">High</option>
                        </select>
                    </div>

                    <button type="submit" class="btn-submit">Submit Complaint</button>
                </form>

                <!-- Success Message (hidden by default) -->
                <div id="successMessage" class="success-card" style="display:none;">
                    <h3>✅ Complaint Submitted Successfully!</h3>
                    <p>Your Complaint ID:</p>
                    <div class="complaint-id" id="generatedId">CMP1001</div>
                    <p>Please save this ID to track your complaint.</p>
                    <button onclick="showSection('track')" class="btn-track">Track My Complaint</button>
                </div>
            </div>
        </div>
    </div>

    <!-- TRACK COMPLAINT SECTION -->
    <div id="track" class="section">
        <div class="container">
            <div class="form-card">
                <h2>🔍 Track Your Complaint</h2>
                <p class="form-subtitle">Enter your Complaint ID to check the current status.</p>

                <div class="track-form">
                    <div class="form-group">
                        <label for="trackId">Complaint ID</label>
                        <input type="text" id="trackId" placeholder="e.g. CMP1001">
                    </div>
                    <button onclick="trackComplaint()" class="btn-submit">Track Complaint</button>
                </div>

                <!-- Track Result (hidden by default) -->
                <div id="trackResult" style="display:none;"></div>
            </div>
        </div>
    </div>

    <!-- Footer -->
    <footer class="footer">
        <p>© 2026 Municipal Corporation - Complaint Management System</p>
        <p>Software Engineering Mini Project</p>
    </footer>

    <script src="/script.js"></script>
</body>
</html>
```

---

## frontend/admin.html

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin Dashboard - Municipal Complaint System</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <!-- Navigation Bar -->
    <nav class="navbar">
        <div class="nav-brand">
            <h1>🏛️ Municipal Corporation</h1>
            <p>Admin Dashboard</p>
        </div>
        <div class="nav-links">
            <a href="/" class="nav-link">Citizen Portal</a>
            <span class="nav-link admin-badge">👤 Municipal Officer</span>
        </div>
    </nav>

    <div class="container admin-container">
        <h2 class="admin-title">📊 Complaint Dashboard</h2>

        <!-- Summary Cards -->
        <div class="stat-cards">
            <div class="stat-card stat-total">
                <h3 id="totalCount">0</h3>
                <p>Total Complaints</p>
            </div>
            <div class="stat-card stat-pending">
                <h3 id="pendingCount">0</h3>
                <p>Pending</p>
            </div>
            <div class="stat-card stat-progress">
                <h3 id="progressCount">0</h3>
                <p>In Progress</p>
            </div>
            <div class="stat-card stat-resolved">
                <h3 id="resolvedCount">0</h3>
                <p>Resolved</p>
            </div>
        </div>

        <!-- Controls (Search and Filters) -->
        <div class="admin-controls">
            <div class="filter-bar">
                <div class="filter-item search-item">
                    <label for="adminSearch">Search Complaints</label>
                    <input type="text" id="adminSearch" placeholder="Search by ID, citizen name, category, location..." oninput="applyFilters()">
                </div>
                <div class="filter-item">
                    <label for="filterStatus">Status</label>
                    <select id="filterStatus" onchange="applyFilters()">
                        <option value="All">All Statuses</option>
                        <option value="Pending">Pending</option>
                        <option value="In Progress">In Progress</option>
                        <option value="Resolved">Resolved</option>
                    </select>
                </div>
                <div class="filter-item">
                    <label for="filterCategory">Category</label>
                    <select id="filterCategory" onchange="applyFilters()">
                        <option value="All">All Categories</option>
                        <option value="Road Damage">Road Damage</option>
                        <option value="Garbage">Garbage</option>
                        <option value="Water Supply">Water Supply</option>
                        <option value="Street Light">Street Light</option>
                        <option value="Drainage">Drainage</option>
                        <option value="Other">Other</option>
                    </select>
                </div>
                <div class="filter-buttons">
                    <button onclick="resetFilters()" class="btn-reset" title="Reset search and filters">Reset</button>
                    <button onclick="loadAdminComplaints()" class="btn-refresh">🔄 Refresh</button>
                </div>
            </div>
        </div>

        <!-- Complaints Table -->
        <div class="table-wrapper">
            <table class="complaint-table">
                <thead>
                    <tr>
                        <th>Complaint ID</th>
                        <th>Citizen</th>
                        <th>Category</th>
                        <th>Location</th>
                        <th>Description</th>
                        <th>Priority</th>
                        <th>Date &amp; Time</th>
                        <th>Status</th>
                        <th>Action</th>
                    </tr>
                </thead>
                <tbody id="complaintsTableBody">
                    <tr>
                        <td colspan="9" style="text-align:center; padding:40px; color:#999;">Loading complaints...</td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>

    <!-- Footer -->
    <footer class="footer">
        <p>© 2026 Municipal Corporation - Admin Dashboard</p>
        <p>Software Engineering Mini Project</p>
    </footer>

    <script src="/script.js"></script>
    <script>
        // Load complaints when admin page loads
        document.addEventListener('DOMContentLoaded', loadAdminComplaints);
    </script>
</body>
</html>
```

---

## frontend/style.css

```css
:root {
  --primary-blue: #1a73e8;
  --dark-blue: #0d47a1;
  --green: #0f9d58;
  --orange: #f4b400;
  --red: #db4437;
  --light-bg: #f8f9fa;
  --text-dark: #202124;
}

* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  color: var(--text-dark);
  background-color: var(--light-bg);
  line-height: 1.6;
}

.navbar {
  background-color: var(--dark-blue);
  color: white;
  padding: 1rem 2rem;
  display: flex;
  justify-content: space-between;
  align-items: center;
  position: sticky;
  top: 0;
  z-index: 1000;
}

.nav-brand h1 {
  font-size: 1.5rem;
  margin-bottom: 0.2rem;
}

.nav-brand p {
  font-size: 0.9rem;
  opacity: 0.8;
}

.nav-links {
  display: flex;
  gap: 1.5rem;
}

.nav-link {
  color: white;
  text-decoration: none;
  padding: 0.5rem 1rem;
  border-radius: 4px;
  transition: background-color 0.3s;
}

.nav-link:hover, .nav-link.active {
  background-color: rgba(255, 255, 255, 0.2);
}

.hero {
  background: linear-gradient(135deg, var(--dark-blue), #00acc1);
  color: white;
  text-align: center;
  padding: 4rem 2rem;
}

.hero h2 {
  font-size: 2.5rem;
  margin-bottom: 1rem;
}

.hero p {
  font-size: 1.2rem;
  margin-bottom: 2rem;
}

.hero-btn {
  background-color: var(--green);
  color: white;
  border: none;
  padding: 1rem 2rem;
  font-size: 1.1rem;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s;
}

.hero-btn:hover {
  background-color: #0d8a4d;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 2rem;
}

.info-cards {
  display: flex;
  gap: 2rem;
  margin-top: -2rem;
  justify-content: center;
}

.info-card {
  background: white;
  padding: 2rem;
  border-radius: 8px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
  text-align: center;
  flex: 1;
}

.info-icon {
  font-size: 2.5rem;
  margin-bottom: 1rem;
}

.form-card {
  background: white;
  padding: 2rem;
  border-radius: 8px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
  max-width: 600px;
  margin: 0 auto;
}

.form-subtitle {
  color: #666;
  margin-bottom: 1.5rem;
}

.form-group {
  margin-bottom: 1.5rem;
}

.form-group label {
  display: block;
  margin-bottom: 0.5rem;
  font-weight: 500;
}

.form-group input,
.form-group select,
.form-group textarea {
  width: 100%;
  padding: 0.8rem;
  border: 1px solid #ccc;
  border-radius: 4px;
  font-size: 1rem;
  transition: border-color 0.3s, box-shadow 0.3s;
}

.form-group input:focus,
.form-group select:focus,
.form-group textarea:focus {
  outline: none;
  border-color: var(--primary-blue);
  box-shadow: 0 0 0 2px rgba(26, 115, 232, 0.2);
}

.btn-submit {
  background-color: var(--green);
  color: white;
  border: none;
  padding: 0.8rem 1.5rem;
  font-size: 1rem;
  border-radius: 4px;
  cursor: pointer;
  width: 100%;
  transition: background-color 0.3s;
}

.btn-submit:hover {
  background-color: #0d8a4d;
}

.success-card {
  margin-top: 2rem;
  padding: 1.5rem;
  background-color: #e6f4ea;
  border-left: 5px solid var(--green);
  border-radius: 4px;
}

.complaint-id {
  font-size: 2rem;
  font-weight: bold;
  color: var(--primary-blue);
  margin: 1rem 0;
}

.btn-track {
  background-color: var(--primary-blue);
  color: white;
  border: none;
  padding: 0.5rem 1rem;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s;
}

.btn-track:hover {
  background-color: var(--dark-blue);
}

.result-card {
  margin-top: 2rem;
  padding: 1.5rem;
  background: white;
  border-radius: 8px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.detail-table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 1rem;
}

.detail-table td {
  padding: 0.8rem;
  border-bottom: 1px solid #eee;
}

.detail-table .label {
  font-weight: 500;
  width: 30%;
  color: #555;
}

.status-badge {
  padding: 0.25rem 0.75rem;
  border-radius: 20px;
  color: white;
  font-size: 0.85rem;
  font-weight: 500;
  display: inline-block;
}

.status-pending {
  background-color: var(--orange);
}

.status-in-progress {
  background-color: var(--primary-blue);
}

.status-resolved {
  background-color: var(--green);
}

/* Priority Badges */
.priority-badge {
  padding: 0.25rem 0.65rem;
  border-radius: 12px;
  font-size: 0.78rem;
  font-weight: 600;
  display: inline-block;
  text-transform: uppercase;
  letter-spacing: 0.4px;
}

.badge-priority-low {
  background-color: #e6f4ea;
  color: #137333;
  border: 1px solid #ceead6;
}

.badge-priority-medium {
  background-color: #fef7e0;
  color: #b06000;
  border: 1px solid #feefc3;
}

.badge-priority-high {
  background-color: #fce8e6;
  color: #c5221f;
  border: 1px solid #fad2cf;
}

/* Visual Status Stepper (Citizen Tracking) */
.stepper-container {
  background: #f8f9fa;
  border: 1px solid #e8eaed;
  border-radius: 8px;
  padding: 1.25rem 1rem;
  margin: 1.5rem 0;
}

.stepper-title {
  text-align: center;
  font-size: 0.85rem;
  color: #5f6368;
  margin-bottom: 1.25rem;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.status-stepper {
  display: flex;
  align-items: center;
  justify-content: space-between;
  max-width: 480px;
  margin: 0 auto;
}

.step {
  display: flex;
  flex-direction: column;
  align-items: center;
  position: relative;
  z-index: 1;
}

.step-circle {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  background: white;
  border: 2px solid #dadce0;
  color: #70757a;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  font-size: 0.82rem;
  transition: all 0.3s;
}

.step-label {
  margin-top: 0.35rem;
  font-size: 0.76rem;
  color: #70757a;
  font-weight: 500;
  white-space: nowrap;
}

.step-line {
  flex: 1;
  height: 3px;
  background-color: #dadce0;
  margin: 0 6px;
  margin-bottom: 16px;
  transition: background-color 0.3s;
}

.step-line.filled {
  background-color: var(--primary-blue);
}

.step.completed .step-circle {
  background-color: var(--green);
  border-color: var(--green);
  color: white;
}

.step.completed .step-label {
  color: var(--green);
  font-weight: 600;
}

.step.active .step-circle {
  background-color: var(--primary-blue);
  border-color: var(--primary-blue);
  color: white;
  box-shadow: 0 0 0 3px rgba(26, 115, 232, 0.2);
}

.step.active .step-label {
  color: var(--primary-blue);
  font-weight: bold;
}

/* Admin Search and Filter Controls */
.filter-bar {
  display: flex;
  gap: 1rem;
  align-items: flex-end;
  flex-wrap: wrap;
  background: white;
  padding: 1rem 1.25rem;
  border-radius: 8px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.06);
  margin-bottom: 1.5rem;
}

.filter-item {
  display: flex;
  flex-direction: column;
  gap: 0.35rem;
  flex: 1;
  min-width: 140px;
}

.filter-item.search-item {
  flex: 2;
  min-width: 220px;
}

.filter-item label {
  font-size: 0.8rem;
  font-weight: 600;
  color: #555;
  text-transform: uppercase;
  letter-spacing: 0.3px;
}

.filter-item input,
.filter-item select {
  padding: 0.5rem 0.75rem;
  border: 1px solid #ccc;
  border-radius: 4px;
  font-size: 0.9rem;
  background-color: #fff;
  transition: border-color 0.2s, box-shadow 0.2s;
}

.filter-item input:focus,
.filter-item select:focus {
  outline: none;
  border-color: var(--primary-blue);
  box-shadow: 0 0 0 2px rgba(26, 115, 232, 0.2);
}

.filter-buttons {
  display: flex;
  gap: 0.5rem;
  align-items: center;
}

.btn-reset {
  background-color: #e8eaed;
  color: #3c4043;
  border: 1px solid #dadce0;
  padding: 0.5rem 0.9rem;
  border-radius: 4px;
  cursor: pointer;
  font-size: 0.9rem;
  font-weight: 500;
  transition: background-color 0.2s;
}

.btn-reset:hover {
  background-color: #dadce0;
}

.date-cell {
  white-space: nowrap;
  font-size: 0.85rem;
  color: #555;
}

.admin-container {
  max-width: 1200px;
}

.admin-title {
  margin-bottom: 2rem;
}

.stat-cards {
  display: flex;
  gap: 1.5rem;
  margin-bottom: 2rem;
}

.stat-card {
  flex: 1;
  padding: 1.5rem;
  border-radius: 8px;
  color: white;
  text-align: center;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.stat-card h3 {
  font-size: 2.5rem;
  margin-bottom: 0.5rem;
}

.stat-total { background-color: var(--primary-blue); }
.stat-pending { background-color: var(--orange); }
.stat-progress { background-color: var(--primary-blue); }
.stat-resolved { background-color: var(--green); }

.admin-controls {
  margin-bottom: 1rem;
}

.btn-refresh {
  background-color: #5f6368;
  color: white;
  border: none;
  padding: 0.5rem 1rem;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s;
}

.btn-refresh:hover {
  background-color: #3c4043;
}

.table-wrapper {
  overflow-x: auto;
  background: white;
  border-radius: 8px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.complaint-table {
  width: 100%;
  border-collapse: collapse;
}

.complaint-table th,
.complaint-table td {
  padding: 1rem;
  text-align: left;
  border-bottom: 1px solid #eee;
}

.complaint-table th {
  background-color: #f8f9fa;
  font-weight: 600;
  color: #333;
}

.complaint-table tbody tr:nth-child(even) {
  background-color: #fafafa;
}

.action-cell {
  display: flex;
  gap: 0.5rem;
  align-items: center;
}

.status-select {
  padding: 0.25rem;
  border-radius: 4px;
  border: 1px solid #ccc;
}

.btn-update {
  background-color: var(--primary-blue);
  color: white;
  border: none;
  padding: 0.25rem 0.5rem;
  border-radius: 4px;
  cursor: pointer;
  font-size: 0.85rem;
}

.error-card {
  margin-top: 2rem;
  padding: 1.5rem;
  background-color: #fce8e6;
  border-left: 5px solid var(--red);
  border-radius: 4px;
}

.section {
  display: none;
}

.section.active {
  display: block;
}

.footer {
  background-color: #202124;
  color: #9aa0a6;
  text-align: center;
  padding: 2rem;
  margin-top: 4rem;
}

.footer p {
  margin-bottom: 0.5rem;
}

@media (max-width: 768px) {
  .navbar {
    flex-direction: column;
    text-align: center;
  }
  .nav-links {
    margin-top: 1rem;
    flex-wrap: wrap;
    justify-content: center;
  }
  .info-cards {
    flex-direction: column;
    margin-top: 2rem;
  }
  .stat-cards {
    flex-direction: column;
  }
}
```

---

## frontend/script.js

```javascript
// ============================================================
// Municipal Complaint Management System - Frontend JavaScript
// ============================================================

// Global cache of all complaints for admin search and filtering
var allComplaints = [];

// ----------------------------------------------------------
// SECTION NAVIGATION (Citizen Page)
// ----------------------------------------------------------

// Show a specific section and hide others
function showSection(sectionId) {
    // Hide all sections
    var sections = document.querySelectorAll('.section');
    for (var i = 0; i < sections.length; i++) {
        sections[i].classList.remove('active');
    }

    // Show the selected section
    var target = document.getElementById(sectionId);
    if (target) {
        target.classList.add('active');
    }

    // Update active nav link
    var links = document.querySelectorAll('.nav-link');
    for (var i = 0; i < links.length; i++) {
        links[i].classList.remove('active');
    }

    // Find the nav link that matches and activate it
    var navLinks = document.querySelectorAll('.nav-links .nav-link');
    for (var i = 0; i < navLinks.length; i++) {
        var onclick = navLinks[i].getAttribute('onclick');
        if (onclick && onclick.indexOf(sectionId) !== -1) {
            navLinks[i].classList.add('active');
        }
    }
}

// ----------------------------------------------------------
// SUBMIT COMPLAINT
// ----------------------------------------------------------

function submitComplaint(event) {
    // Prevent the form from reloading the page
    event.preventDefault();

    // Get form values
    var name = document.getElementById('name').value.trim();
    var phone = document.getElementById('phone').value.trim();
    var category = document.getElementById('category').value;
    var description = document.getElementById('description').value.trim();
    var location = document.getElementById('location').value.trim();
    var priorityEl = document.getElementById('priority');
    var priority = priorityEl ? priorityEl.value : 'Medium';

    // Validate all required fields
    if (!name || !phone || !category || !description || !location) {
        alert('Please fill in all required fields.');
        return;
    }

    // Create the request body
    var data = {
        name: name,
        phone: phone,
        category: category,
        description: description,
        location: location,
        priority: priority
    };

    // Send POST request to the server
    fetch('/api/complaints', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    })
    .then(function(response) { return response.json(); })
    .then(function(result) {
        if (result.success) {
            // Show success message with the generated complaint ID
            document.getElementById('complaintForm').style.display = 'none';
            document.getElementById('generatedId').textContent = result.complaintId;
            document.getElementById('successMessage').style.display = 'block';

            // Reset the form for next use
            document.getElementById('complaintForm').reset();
            if (priorityEl) priorityEl.value = 'Medium';
        } else {
            alert('Error: ' + result.message);
        }
    })
    .catch(function(error) {
        alert('Failed to submit complaint. Please try again.');
        console.error('Error:', error);
    });
}

// ----------------------------------------------------------
// TRACK COMPLAINT
// ----------------------------------------------------------

// Helper function to build visual 4-step status progression
function getStatusStepperHtml(currentStatus) {
    var stepSubmittedClass = 'step completed';
    var stepPendingClass = 'step';
    var stepProgressClass = 'step';
    var stepResolvedClass = 'step';

    if (currentStatus === 'Pending') {
        stepPendingClass += ' active';
    } else if (currentStatus === 'In Progress') {
        stepPendingClass += ' completed';
        stepProgressClass += ' active';
    } else if (currentStatus === 'Resolved') {
        stepPendingClass += ' completed';
        stepProgressClass += ' completed';
        stepResolvedClass += ' active completed';
    }

    var line1 = (stepPendingClass.indexOf('completed') !== -1 || stepPendingClass.indexOf('active') !== -1);
    var line2 = (stepProgressClass.indexOf('completed') !== -1 || stepProgressClass.indexOf('active') !== -1);
    var line3 = (stepResolvedClass.indexOf('completed') !== -1);

    var html = '<div class="stepper-container">' +
        '<h4 class="stepper-title">Resolution Progress</h4>' +
        '<div class="status-stepper">' +
            '<div class="' + stepSubmittedClass + '">' +
                '<div class="step-circle">✓</div>' +
                '<div class="step-label">Submitted</div>' +
            '</div>' +
            '<div class="step-line ' + (line1 ? 'filled' : '') + '"></div>' +
            '<div class="' + stepPendingClass + '">' +
                '<div class="step-circle">' + (stepPendingClass.indexOf('completed') !== -1 ? '✓' : '1') + '</div>' +
                '<div class="step-label">Pending</div>' +
            '</div>' +
            '<div class="step-line ' + (line2 ? 'filled' : '') + '"></div>' +
            '<div class="' + stepProgressClass + '">' +
                '<div class="step-circle">' + (stepProgressClass.indexOf('completed') !== -1 ? '✓' : '2') + '</div>' +
                '<div class="step-label">In Progress</div>' +
            '</div>' +
            '<div class="step-line ' + (line3 ? 'filled' : '') + '"></div>' +
            '<div class="' + stepResolvedClass + '">' +
                '<div class="step-circle">' + (stepResolvedClass.indexOf('completed') !== -1 ? '✓' : '3') + '</div>' +
                '<div class="step-label">Resolved</div>' +
            '</div>' +
        '</div>' +
    '</div>';

    return html;
}

function trackComplaint() {
    var trackInput = document.getElementById('trackId').value.trim();

    if (!trackInput) {
        alert('Please enter a Complaint ID.');
        return;
    }

    // Extract numeric ID from input (handles both "CMP1001" and "1001")
    var numericId = trackInput.replace(/[^0-9]/g, '');

    if (!numericId) {
        alert('Please enter a valid Complaint ID (e.g., CMP1001).');
        return;
    }

    // Fetch the complaint from the server
    fetch('/api/complaints/' + numericId)
    .then(function(response) {
        if (response.status === 404) {
            // Complaint not found
            document.getElementById('trackResult').innerHTML =
                '<div class="error-card">' +
                '<h3>❌ Complaint Not Found</h3>' +
                '<p>Please check your Complaint ID and try again.</p>' +
                '</div>';
            document.getElementById('trackResult').style.display = 'block';
            return null;
        }
        return response.json();
    })
    .then(function(complaint) {
        if (!complaint) return;

        // Determine status badge class
        var statusClass = 'status-pending';
        if (complaint.status === 'In Progress') statusClass = 'status-in-progress';
        if (complaint.status === 'Resolved') statusClass = 'status-resolved';

        // Determine priority badge class
        var priorityClass = 'badge-priority-medium';
        if (complaint.priority === 'Low') priorityClass = 'badge-priority-low';
        if (complaint.priority === 'High') priorityClass = 'badge-priority-high';

        // Display complaint details card with status stepper
        var html =
            '<div class="result-card">' +
            '<h3>Complaint Status Details</h3>' +
            getStatusStepperHtml(complaint.status) +
            '<table class="detail-table">' +
            '<tr><td class="label">Complaint ID</td><td><strong>' + complaint.complaintId + '</strong></td></tr>' +
            '<tr><td class="label">Citizen Name</td><td>' + complaint.name + '</td></tr>' +
            '<tr><td class="label">Phone</td><td>' + complaint.phone + '</td></tr>' +
            '<tr><td class="label">Category</td><td>' + complaint.category + '</td></tr>' +
            '<tr><td class="label">Description</td><td>' + complaint.description + '</td></tr>' +
            '<tr><td class="label">Location</td><td>' + complaint.location + '</td></tr>' +
            '<tr><td class="label">Priority</td><td><span class="priority-badge ' + priorityClass + '">' + (complaint.priority || 'Medium') + '</span></td></tr>' +
            '<tr><td class="label">Date &amp; Time</td><td>' + (complaint.dateTime || 'Not Available') + '</td></tr>' +
            '<tr><td class="label">Current Status</td><td><span class="status-badge ' + statusClass + '">' + complaint.status + '</span></td></tr>' +
            '</table>' +
            '</div>';

        document.getElementById('trackResult').innerHTML = html;
        document.getElementById('trackResult').style.display = 'block';
    })
    .catch(function(error) {
        alert('Failed to track complaint. Please try again.');
        console.error('Error:', error);
    });
}

// ----------------------------------------------------------
// ADMIN DASHBOARD
// ----------------------------------------------------------

function loadAdminComplaints() {
    // Fetch all complaints
    fetch('/api/complaints')
    .then(function(response) { return response.json(); })
    .then(function(complaints) {
        // Cache in global array for client-side search and filtering
        allComplaints = complaints;

        // Calculate statistics based on full data
        var total = complaints.length;
        var pending = 0;
        var inProgress = 0;
        var resolved = 0;

        for (var i = 0; i < complaints.length; i++) {
            if (complaints[i].status === 'Pending') pending++;
            else if (complaints[i].status === 'In Progress') inProgress++;
            else if (complaints[i].status === 'Resolved') resolved++;
        }

        // Update stat cards
        var totalEl = document.getElementById('totalCount');
        var pendingEl = document.getElementById('pendingCount');
        var progressEl = document.getElementById('progressCount');
        var resolvedEl = document.getElementById('resolvedCount');

        if (totalEl) totalEl.textContent = total;
        if (pendingEl) pendingEl.textContent = pending;
        if (progressEl) progressEl.textContent = inProgress;
        if (resolvedEl) resolvedEl.textContent = resolved;

        // Apply any active search query and dropdown filters
        applyFilters();
    })
    .catch(function(error) {
        console.error('Error loading complaints:', error);
    });
}

// Filter the cached complaints list by search keyword, status, and category
function applyFilters() {
    var searchInput = document.getElementById('adminSearch');
    var statusSelect = document.getElementById('filterStatus');
    var categorySelect = document.getElementById('filterCategory');

    var query = searchInput ? searchInput.value.trim().toLowerCase() : '';
    var statusFilter = statusSelect ? statusSelect.value : 'All';
    var categoryFilter = categorySelect ? categorySelect.value : 'All';

    var filtered = allComplaints.filter(function(c) {
        // Search across ID, citizen name, category, and location
        var matchesSearch = true;
        if (query) {
            var idStr = (c.complaintId || ('CMP' + c.id)).toLowerCase();
            var nameStr = (c.name || '').toLowerCase();
            var catStr = (c.category || '').toLowerCase();
            var locStr = (c.location || '').toLowerCase();

            matchesSearch = (idStr.indexOf(query) !== -1 ||
                             nameStr.indexOf(query) !== -1 ||
                             catStr.indexOf(query) !== -1 ||
                             locStr.indexOf(query) !== -1);
        }

        // Filter by Status
        var matchesStatus = (statusFilter === 'All' || c.status === statusFilter);

        // Filter by Category
        var matchesCategory = (categoryFilter === 'All' || c.category === categoryFilter);

        return matchesSearch && matchesStatus && matchesCategory;
    });

    renderComplaintsTable(filtered);
}

// Reset all search and filter controls to default
function resetFilters() {
    var searchInput = document.getElementById('adminSearch');
    var statusSelect = document.getElementById('filterStatus');
    var categorySelect = document.getElementById('filterCategory');

    if (searchInput) searchInput.value = '';
    if (statusSelect) statusSelect.value = 'All';
    if (categorySelect) categorySelect.value = 'All';

    applyFilters();
}

// Render the complaints table rows with Priority and Date & Time columns
function renderComplaintsTable(complaints) {
    var tbody = document.getElementById('complaintsTableBody');
    if (!tbody) return;

    if (allComplaints.length === 0) {
        tbody.innerHTML = '<tr><td colspan="9" style="text-align:center;padding:40px;color:#777;">No complaints registered yet.</td></tr>';
        return;
    }

    if (complaints.length === 0) {
        tbody.innerHTML = '<tr><td colspan="9" style="text-align:center;padding:40px;color:#777;">No complaints match your search or filter criteria.</td></tr>';
        return;
    }

    var html = '';
    for (var i = 0; i < complaints.length; i++) {
        var c = complaints[i];

        // Status badge class
        var statusClass = 'status-pending';
        if (c.status === 'In Progress') statusClass = 'status-in-progress';
        if (c.status === 'Resolved') statusClass = 'status-resolved';

        // Priority badge class
        var priorityClass = 'badge-priority-medium';
        if (c.priority === 'Low') priorityClass = 'badge-priority-low';
        if (c.priority === 'High') priorityClass = 'badge-priority-high';

        html += '<tr>';
        html += '<td><strong>' + c.complaintId + '</strong></td>';
        html += '<td>' + c.name + '<br><small>' + c.phone + '</small></td>';
        html += '<td>' + c.category + '</td>';
        html += '<td>' + c.location + '</td>';
        html += '<td class="desc-cell">' + c.description + '</td>';
        html += '<td><span class="priority-badge ' + priorityClass + '">' + (c.priority || 'Medium') + '</span></td>';
        html += '<td class="date-cell">' + (c.dateTime || 'Not Available') + '</td>';
        html += '<td><span class="status-badge ' + statusClass + '">' + c.status + '</span></td>';
        html += '<td class="action-cell">';
        html += '<select id="status-' + c.id + '" class="status-select">';
        html += '<option value="Pending"' + (c.status === 'Pending' ? ' selected' : '') + '>Pending</option>';
        html += '<option value="In Progress"' + (c.status === 'In Progress' ? ' selected' : '') + '>In Progress</option>';
        html += '<option value="Resolved"' + (c.status === 'Resolved' ? ' selected' : '') + '>Resolved</option>';
        html += '</select>';
        html += '<button onclick="updateStatus(' + c.id + ')" class="btn-update">Update</button>';
        html += '</td>';
        html += '</tr>';
    }
    tbody.innerHTML = html;
}

// Update complaint status from admin
function updateStatus(id) {
    var selectEl = document.getElementById('status-' + id);
    if (!selectEl) return;

    var newStatus = selectEl.value;

    fetch('/api/complaints/' + id + '/status', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ status: newStatus })
    })
    .then(function(response) { return response.json(); })
    .then(function(result) {
        if (result.success) {
            // Reload complaints to refresh the table and summary counts
            loadAdminComplaints();
        } else {
            alert('Error: ' + result.message);
        }
    })
    .catch(function(error) {
        alert('Failed to update status.');
        console.error('Error:', error);
    });
}
```
