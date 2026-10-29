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
