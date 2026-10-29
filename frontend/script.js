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
