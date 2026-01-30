#!/usr/bin/env python3
"""
Simple HTTP server for testing the Leaflet map implementation
This serves the HTML file and provides basic API endpoints for testing
"""

import http.server
import socketserver
import json
import urllib.parse
from pathlib import Path

PORT = 8080

# Sample venues data for testing - Bangalore locations
venues_data = [
    {"id": 1, "name": "PVR Forum Mall", "type": "cinema", "x": 200, "y": 150},
    {"id": 2, "name": "Toit Brewpub", "type": "restaurant", "x": 400, "y": 200},
    {"id": 3, "name": "Cubbon Park", "type": "park", "x": 300, "y": 100},
    {"id": 4, "name": "UB City Mall", "type": "mall", "x": 500, "y": 300},
    {"id": 5, "name": "Ranga Shankara", "type": "theater", "x": 150, "y": 250},
    {"id": 6, "name": "Koshy's Restaurant", "type": "restaurant", "x": 350, "y": 350},
    {"id": 7, "name": "INOX Garuda Mall", "type": "cinema", "x": 600, "y": 100},
    {"id": 8, "name": "Lalbagh Gardens", "type": "park", "x": 100, "y": 400},
    {"id": 9, "name": "Bangalore Palace", "type": "theater", "x": 250, "y": 180},
    {"id": 10, "name": "Commercial Street", "type": "mall", "x": 320, "y": 280},
    {"id": 11, "name": "Truffles", "type": "restaurant", "x": 450, "y": 120},
    {"id": 12, "name": "Ulsoor Lake", "type": "park", "x": 380, "y": 220},
]

class CustomHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path.startswith('/api/venues'):
            self.handle_venues_api()
        elif self.path.startswith('/api/nearest'):
            self.handle_nearest_api()
        elif self.path.startswith('/api/tree'):
            self.handle_tree_api()
        elif self.path.startswith('/api/add_venue'):
            self.handle_add_venue_api()
        elif self.path.startswith('/api/delete_venue'):
            self.handle_delete_venue_api()
        else:
            # Serve static files
            super().do_GET()

    def handle_venues_api(self):
        response = {"venues": venues_data}
        self.send_json_response(response)

    def handle_nearest_api(self):
        # Parse query parameters
        query = urllib.parse.urlparse(self.path).query
        params = urllib.parse.parse_qs(query)
        
        x = float(params.get('x', [0])[0])
        y = float(params.get('y', [0])[0])
        venue_type = params.get('type', ['all'])[0]
        
        # Filter venues by type
        filtered_venues = venues_data
        if venue_type != 'all':
            filtered_venues = [v for v in venues_data if v['type'] == venue_type]
        
        if not filtered_venues:
            response = {"success": False, "message": "No venues found"}
        else:
            # Find nearest venue (simple Euclidean distance)
            nearest = min(filtered_venues, 
                         key=lambda v: ((v['x'] - x) ** 2 + (v['y'] - y) ** 2) ** 0.5)
            
            response = {
                "success": True,
                "result": nearest,
                "searches": len(filtered_venues),  # Simulated search count
                "backend": "python",
                "complexity": "O(n)"
            }
        
        self.send_json_response(response)

    def handle_tree_api(self):
        tree_text = """K-D Tree Structure (Bangalore Venues):
Root: Toit Brewpub (400, 200)
├── Left: Cubbon Park (300, 100)
│   ├── Left: PVR Forum Mall (200, 150)
│   │   └── Left: Ranga Shankara (150, 250)
│   └── Right: Bangalore Palace (250, 180)
└── Right: UB City Mall (500, 300)
    ├── Left: Truffles (450, 120)
    │   └── Right: INOX Garuda Mall (600, 100)
    └── Right: Koshy's Restaurant (350, 350)
        ├── Left: Commercial Street (320, 280)
        │   └── Left: Ulsoor Lake (380, 220)
        └── Right: Lalbagh Gardens (100, 400)

Note: This is a simplified visualization for testing.
The actual C backend would show the real tree structure based on spatial partitioning."""
        
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(tree_text.encode())

    def handle_add_venue_api(self):
        query = urllib.parse.urlparse(self.path).query
        params = urllib.parse.parse_qs(query)
        
        name = params.get('name', [''])[0]
        venue_type = params.get('type', [''])[0]
        x = float(params.get('x', [0])[0])
        y = float(params.get('y', [0])[0])
        
        if name:
            new_id = max([v['id'] for v in venues_data]) + 1
            new_venue = {"id": new_id, "name": name, "type": venue_type, "x": x, "y": y}
            venues_data.append(new_venue)
            
            response = {"success": True, "id": new_id, "message": "Venue added"}
        else:
            response = {"success": False, "message": "Name required"}
        
        self.send_json_response(response)

    def handle_delete_venue_api(self):
        query = urllib.parse.urlparse(self.path).query
        params = urllib.parse.parse_qs(query)
        
        venue_id = int(params.get('id', [0])[0])
        
        global venues_data
        original_count = len(venues_data)
        venues_data = [v for v in venues_data if v['id'] != venue_id]
        
        if len(venues_data) < original_count:
            response = {"success": True, "message": "Venue deleted"}
        else:
            response = {"success": False, "message": "Venue not found"}
        
        self.send_json_response(response)

    def send_json_response(self, data):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), CustomHTTPRequestHandler) as httpd:
        print(f"Server running at http://localhost:{PORT}")
        print("Serving the Leaflet map implementation...")
        print("Open http://localhost:8080/index.html in your browser")
        httpd.serve_forever()