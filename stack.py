import sys
import re
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict

def parse_ns2_trace(filepath):
    """
    Parses the ns2mobility.tcl file to find:
    1. Initial positions (set X_, set Y_)
    2. Entry times (first time a node is mentioned in a movement command)
    """
    node_positions = {} # {node_id: {'x': val, 'y': val}}
    entry_times = {}    # {node_id: first_movement_time}
    
    # Regex patterns for NS2 format
    pos_pattern = re.compile(r'\$node_\((\d+)\)\s+set\s+([XYZ])_\s+([\d\.]+)')
    move_pattern = re.compile(r'\$ns_\s+at\s+([\d\.]+)\s+"\$node_\((\d+)\)\s+setdest')

    try:
        with open(filepath, 'r') as f:
            for line in f:
                # 1. Check for Initial Position
                pos_match = pos_pattern.search(line)
                if pos_match:
                    node_id = int(pos_match.group(1))
                    axis = pos_match.group(2)
                    coord = float(pos_match.group(3))
                    
                    if node_id not in node_positions:
                        node_positions[node_id] = {'x': 0.0, 'y': 0.0}
                    
                    if axis == 'X':
                        node_positions[node_id]['x'] = coord
                    elif axis == 'Y':
                        node_positions[node_id]['y'] = coord
                        
                # 2. Check for Entry Time (First movement)
                move_match = move_pattern.search(line)
                if move_match:
                    time = float(move_match.group(1))
                    node_id = int(move_match.group(2))
                    
                    if node_id not in entry_times:
                        entry_times[node_id] = time
                    else:
                        entry_times[node_id] = min(entry_times[node_id], time)

    except FileNotFoundError:
        print(f"Error: File '{filepath}' not found.")
        sys.exit(1)

    return node_positions, entry_times

def plot_ghost_nodes(node_positions, entry_times):
    x_vals = []
    y_vals = []
    wait_times = []
    ids = []

    print(f"Analyzing {len(node_positions)} nodes...")
    
    # Detect Clusters
    location_counts = defaultdict(int)

    for nid, pos in node_positions.items():
        start_time = entry_times.get(nid, 0.0)
        
        x_vals.append(pos['x'])
        y_vals.append(pos['y'])
        wait_times.append(start_time)
        ids.append(nid)
        
        loc_key = (round(pos['x'], 1), round(pos['y'], 1))
        location_counts[loc_key] += 1

    # --- Print Text Analysis to Console ---
    print("\n--- SPAWN POINT DENSITY ANALYSIS ---")
    print(f"{'Location (X, Y)':<25} | {'Count (Nodes Stacked)':<15}")
    print("-" * 45)
    for loc, count in location_counts.items():
        if count > 1:
            print(f"{str(loc):<25} | {count:<15} <--- Potential Jammer!")
            
    # --- Generate Plot ---
    plt.figure(figsize=(12, 8))
    
    # Scatter plot: Color = Wait Time (Ghost Duration)
    sc = plt.scatter(x_vals, y_vals, c=wait_times, cmap='plasma', s=150, edgecolors='black', alpha=0.8)
    
    # Add Colorbar
    cbar = plt.colorbar(sc)
    cbar.set_label('Ghost Duration (seconds)\n(Time spent sitting idle)', fontsize=12)

    # REMOVED: The annotation loop that displayed "N{id}" is gone.

    plt.title('Visualization of Vehicles when App starts at t=0)', fontsize=14)
    plt.xlabel('X Position (meters)', fontsize=12)
    plt.ylabel('Y Position (meters)', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.5)
    
    output_filename = 'ghost_node_analysis.png'
    plt.savefig(output_filename)
    print(f"\nGraph saved to: {output_filename}")
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 visualize_ghost_nodes_clean.py <path_to_ns2mobility.tcl>")
        trace_file = "sumo/ns2mobility.tcl" 
    else:
        trace_file = sys.argv[1]

    nodes, times = parse_ns2_trace(trace_file)
    plot_ghost_nodes(nodes, times)