# Municipal Complaint Management System

## 🏛️ Project Title
**Complaint Management System for Municipal Corporation**

## 📋 Project Objective
This web application allows citizens to submit civic complaints to a Municipal Corporation and enables municipal officers to view, search, filter, manage, and update those complaints. The system provides an easy and transparent way to track civic complaint resolution.

## ✨ Features
1. **Citizen Complaint Submission** – Citizens can submit complaints with details like name, phone, category, description, location, and priority.
2. **Auto-Generated Complaint ID** – Each complaint gets a unique sequential ID starting from `1001` (e.g., CMP1001, CMP1002).
3. **Automatic Date & Time Stamping** – The C++ backend automatically timestamps each complaint with the current date and time (`DD-MM-YYYY HH:MM`).
4. **Complaint Priority Selection** – Citizens can set priority (`Low`, `Medium`, `High`) with visual color-coded priority badges.
5. **Interactive Complaint Tracking** – Citizens can track their complaint status using the complaint ID, showing full complaint details and an intuitive **4-step visual resolution progress bar** (Submitted → Pending → In Progress → Resolved).
6. **Admin Dashboard with Live Statistics** – Municipal officers can view summary counters for Total Complaints, Pending, In Progress, and Resolved.
7. **Admin Search & Multi-Criteria Filtering** – Officers can search complaints in real time by ID, citizen name, category, or location, and filter by status and category.
8. **Status Management** – Officers can update complaint status (`Pending` → `In Progress` → `Resolved`).
9. **Safe Persistent CSV Storage** – All data is stored in `data/complaints.csv` with RFC 4180 escaping and persists across server restarts.
10. **Backward Compatibility** – Handles older 7-column CSV records safely with sensible defaults (`Medium` priority and `Not Available` timestamp).

## 🛠️ Technology Used

| Component | Technology |
|-----------|------------|
| Backend | C++17 |
| HTTP Server | cpp-httplib (single-header library) |
| Frontend | HTML5, CSS3, Vanilla JavaScript |
| Data Storage | CSV File (`data/complaints.csv`) |
| Compiler | g++ (MinGW / w64devkit) |

**No database, no frameworks, no Node.js, no React.**

## 📁 Project Structure

```
MunicipalComplaintSystem/
│
├── backend/
│   ├── main.cpp          ← C++ web server (routes & API endpoints)
│   ├── complaint.h       ← Complaint structure & function declarations
│   ├── complaint.cpp     ← Complaint CRUD operations (CSV read/write & timestamps)
│   └── httplib.h         ← cpp-httplib single-header HTTP library
│
├── frontend/
│   ├── index.html        ← Citizen-facing portal
│   ├── admin.html        ← Admin/Officer dashboard with search & filters
│   ├── style.css         ← Stylesheet (stepper, priority badges, filters, responsive table)
│   └── script.js         ← Frontend JavaScript (fetch API calls, filtering, stepper UI)
│
├── data/
│   └── complaints.csv    ← Persistent 9-column complaint data storage
│
├── complaint_server.exe  ← Standalone compiled executable
├── PROJECT_SOURCE_CODE.md← Preserved source code reference
└── README.md             ← This file
```

## ⚙️ How the System Works

```
Citizen submits complaint via web form (with priority)
        ↓
JavaScript sends POST request to C++ server
        ↓
Server generates unique Complaint ID (CMP1001, CMP1002, ...)
Server automatically stamps current Date & Time (DD-MM-YYYY HH:MM)
        ↓
Complaint is saved to data/complaints.csv (9 columns)
        ↓
Success response sent back with Complaint ID
        ↓
Citizen tracks complaint using the ID
(Sees details, priority badge, date/time, and visual status progress bar)
        ↓
Municipal Officer opens Admin Dashboard
(Sees summary statistics, uses search box and status/category filters)
        ↓
Officer updates status (Pending → In Progress → Resolved)
        ↓
Status update saved to CSV file
        ↓
Citizen tracks complaint again and sees updated progress
```

## 🔨 How to Compile

Open a terminal (PowerShell or Command Prompt) in the **project root directory** (`MunicipalComplaintSystem/`).

### Windows (MinGW g++):

```powershell
g++ -std=c++17 backend/main.cpp backend/complaint.cpp -o complaint_server.exe -lws2_32
```

> **Note:** The `-lws2_32` flag is required on Windows for socket/networking support used by cpp-httplib.

### Linux / macOS:

```bash
g++ -std=c++17 backend/main.cpp backend/complaint.cpp -o complaint_server -lpthread
```

## 🚀 How to Run

### Step 1: Start the Server

From the **project root directory**:

```powershell
.\complaint_server.exe
```

You will see:

```
===================================
  Municipal Complaint System
  Server starting...
===================================

Server is running at: http://localhost:8081
Citizen Portal:       http://localhost:8081
Admin Dashboard:      http://localhost:8081/admin

Press Ctrl+C to stop the server.
===================================
```

### Step 2: Open the Website

| Page | URL |
|------|-----|
| Citizen Portal | [http://localhost:8081](http://localhost:8081) |
| Admin Dashboard | [http://localhost:8081/admin](http://localhost:8081/admin) |

## 🔌 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/` | Serve citizen portal (index.html) |
| `GET` | `/admin` | Serve admin dashboard (admin.html) |
| `GET` | `/style.css` | Serve stylesheet |
| `GET` | `/script.js` | Serve client-side JavaScript |
| `POST` | `/api/complaints` | Submit a new complaint (stores priority & auto date/time) |
| `GET` | `/api/complaints` | Get all complaints (JSON array with priority and dateTime) |
| `GET` | `/api/complaints/{id}` | Get a specific complaint by numeric ID |
| `PUT` | `/api/complaints/{id}/status` | Update complaint status |

### POST /api/complaints

**Request Body:**
```json
{
    "name": "Rahul Sharma",
    "phone": "DEMO_PHONE_001",
    "category": "Road Damage",
    "description": "Large pothole on Main Street near bus stop",
    "location": "Main Street, Sector 5",
    "priority": "High"
}
```

**Response:**
```json
{
    "success": true,
    "complaintId": "CMP1001",
    "message": "Complaint submitted successfully"
}
```

### GET /api/complaints/{id}

**Response:**
```json
{
    "id": 1001,
    "complaintId": "CMP1001",
    "name": "Rahul Sharma",
    "phone": "DEMO_PHONE_001",
    "category": "Road Damage",
    "description": "Large pothole on Main Street near bus stop",
    "location": "Main Street, Sector 5",
    "priority": "High",
    "dateTime": "06-10-2026 20:45",
    "status": "Pending"
}
```

### PUT /api/complaints/{id}/status

**Request Body:**
```json
{
    "status": "In Progress"
}
```

**Response:**
```json
{
    "success": true,
    "message": "Status updated to In Progress"
}
```

## 📊 CSV Storage Format

File: `data/complaints.csv`

```csv
id,name,phone,category,description,location,priority,date_time,status
```

Columns:
1. `id` – Numeric complaint ID (starts from 1001)
2. `name` – Citizen's full name
3. `phone` – Contact number
4. `category` – Road Damage, Garbage, Water Supply, Street Light, Drainage, Other
5. `description` – Detailed text describing the issue (RFC 4180 quotes supported)
6. `location` – Physical location
7. `priority` – Low, Medium, High
8. `date_time` – Submission timestamp formatted as `DD-MM-YYYY HH:MM`
9. `status` – Pending, In Progress, Resolved

## 📝 Sample Workflow

1. Open **http://localhost:8081** in your browser.
2. Click **"Submit a Complaint"**.
3. Fill in the form:
   - Name: Rahul Sharma
   - Phone: DEMO_PHONE_001
   - Category: Road Damage
   - Description: Large pothole near the bus stop
   - Location: Main Street, Sector 5
   - Priority: High
4. Click **Submit Complaint**.
5. Save the generated **Complaint ID** (e.g., CMP1001).
6. Click **Track Complaint** and enter the ID.
7. Verify complaint details, priority badge (`High`), date/time stamp, and the **Resolution Progress** bar showing **Pending** highlighted.
8. Open **http://localhost:8081/admin**.
9. Notice the complaint in the dashboard table with its priority badge and date/time.
10. Test the **Search Complaints** box (type name or location) to filter in real-time.
11. Test the **Status** and **Category** dropdown filters.
12. Change the status dropdown to **"In Progress"** and click **Update**.
13. Return to the citizen portal and track again: the **In Progress** step is now active.
14. In admin dashboard, change status to **"Resolved"**.
15. Track again: the **Resolved** step turns green and completed! ✅

---

**Developed as a Software Engineering Mini Project**
