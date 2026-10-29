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
