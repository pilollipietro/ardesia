#!/usr/bin/env python3
import time
import math
import subprocess
import os
import math
from dogtail import rawinput, tree, config

config.config.defaultDelay = 0.01


def generate_ellipse_path(bounding_box, scale=0.5):
    """
    Generates a path for an ellipse and its center coordinates.

    The ellipse is centered within the bounding_box and scaled by the
    given factor.

    Args:
        bounding_box (tuple): The (x, y, w, h) geometry of the full area.
        scale (float): A factor to scale the ellipse's size (e.g., 1.0 for
                       the full area, 0.5 for the central half).

    Returns:
        tuple: A tuple containing two elements:
               1. A list of (x, y) integer tuples for the path.
               2. A tuple (center_x, center_y) for the ellipse's center.
    """
    # 1. Calculate the effective bounding box based on the scale factor
    x, y, w, h = bounding_box   
    
    effective_w = w * scale
    effective_h = h * scale
    
    effective_x = x + (w - effective_w) / 2
    effective_y = y + (h - effective_h) / 2
    
    # 2. The rest of the logic uses the new, effective dimensions
    num_points = 100
    center_x = effective_x + effective_w / 2
    center_y = effective_y + effective_h / 2
    radius_x = effective_w / 2
    radius_y = effective_h / 2
    path = []

    for i in range(num_points + 1):
        progress = i / num_points
        theta = progress * 2 * math.pi
        
        point_x = center_x + radius_x * math.cos(theta)
        point_y = center_y + radius_y * math.sin(theta)
        
        path.append((int(point_x), int(point_y)))

    # 3. Return both the path and the center coordinates
    center_coords = (int(center_x), int(center_y))
    return path, center_coords

def generate_rose_curve_path(bounding_box, k=5):
    """
    Generates a path for a Rose Curve (flower shape) that fits inside
    the bounding_box, tracing the path only once.

    Args:
        bounding_box (tuple): The (x, y, w, h) geometry of the target area.
        k (int): Parameter for the number of petals.
                 If k is odd, the number of petals is k.
                 If k is even, the number of petals will be 2*k.

    Returns:
        list: A list of (x, y) integer tuples for the path.
    """
    x, y, w, h = bounding_box
    
    # Parameters
    num_points = 200
    center_x = x + w / 2
    center_y = y + h / 2
    max_radius = min(w, h) / 2
    
    path = []

    # Use the correct angular range to trace the curve only once:
    # PI (180 degrees) for odd k, 2*PI (360 degrees) for even k.
    angular_range = math.pi if k % 2 != 0 else 2 * math.pi
    
    for i in range(num_points + 1):
        progress = i / num_points
        theta = progress * angular_range
        
        # Polar equation for a Rose Curve
        radius = max_radius * math.cos(k * theta)
        
        # Convert polar to Cartesian coordinates and translate to center
        point_x = center_x + radius * math.cos(theta)
        point_y = center_y + radius * math.sin(theta)
        
        path.append((int(point_x), int(point_y)))
        
    return path
    
def generate_tilde_path(bounding_box):
    """Generates a list of points forming a tilde shape.

    The generated tilde is horizontally centered and occupies half the width
    of the provided bounding box. The wave shape is calculated using a cosine
    function to ensure a smooth curve.

    Args:
        bounding_box (tuple): A tuple of four integers (x, y, w, h) representing
            the geometry of the target area, where (x, y) is the top-left
            corner and (w, h) are the width and height.

    Returns:
        list: A list of (x, y) integer tuples representing the points on the
            tilde path.
    """
    (x, y, w, h) = bounding_box
    # --- Parameters you can tweak ---
    # Number of points to define the curve's smoothness
    num_points = 40
    # How "tall" the tilde is relative to its width
    amplitude_ratio = 0.15

    # --- Geometric calculations ---
    # Calculate the center and dimensions of the tilde
    center_x = x + w / 2
    center_y = y + h / 2
    tilde_width = w / 2
    tilde_amplitude = tilde_width * amplitude_ratio

    # Calculate the starting x-coordinate
    start_x = center_x - tilde_width / 2
    
    path = []
    
    # Cosine angles are chosen to ensure the curve starts, crosses,
    # and ends at the vertical center, like a proper tilde.
    start_angle = math.pi / 2
    end_angle = 5 * math.pi / 2 # Exactly one full cycle after the start
    angle_range = end_angle - start_angle

    # --- Point generation ---
    for i in range(num_points):
        # Calculate progress from 0.0 to 1.0
        progress = i / (num_points - 1)
        
        # X-coordinate moves linearly from left to right
        current_x = start_x + progress * tilde_width
        
        # Y-coordinate follows the cosine wave
        angle = start_angle + progress * angle_range
        y_offset = tilde_amplitude * math.cos(angle)
        
        # We subtract the offset to make it go up first, then down
        current_y = center_y - y_offset
        
        # Add the point to the path as a tuple of integers
        path.append((int(current_x), int(current_y)))
        
    return path

def clear_screen(ardesia_window):
    btn = ardesia_window.child("Clear")
    btn.click()
    
# === Helpers ===
def push_button(ardesia_window, button_id):
    btn = ardesia_window.child(button_id)
    btn.click()

def select_color(ardesia_window, button_id):
    btn = ardesia_window.child(button_id)
    btn.click()

def simulate_drawing(path):
    for n, point in enumerate(path):
        rawinput.absoluteMotion(point[0], point[1]);
        if (n == 0):
            rawinput.press(point[0], point[1])
        if (n == len(path)-1):
            rawinput.release(point[0], point[1])
            
def switch_thickness(ardesia_window):
    push_button(ardesia_window, "Thickness")

def switch_draw_mode(ardesia_window):
    push_button(ardesia_window, "Align to Shape") 
    
def fill(ardesia_window, x, y):
    time.sleep(3)
    push_button(ardesia_window, "Bucket Fill")
    rawinput.click(x, y, 1)
    time.sleep(3)
    
def undo(ardesia_window):
    push_button(ardesia_window, "Undo")
 
def redo(ardesia_window):
    push_button(ardesia_window, "Redo")

def hide_unhide(ardesia_window):
    push_button(ardesia_window, "Hide")
    time.sleep(1)
    push_button(ardesia_window, "Hide")
      
# === Scenarios ===
def scenario_freehand(ardesia_window, bounding_box):
    clear_screen(ardesia_window)
    print("[*] Scenario FreeHand")
    push_button(ardesia_window, "Pencil")
    
    select_color(ardesia_window, "Red")
    switch_thickness(ardesia_window)
    path = generate_tilde_path(bounding_box)
    simulate_drawing(path)
    
    select_color(ardesia_window, "Green")
    switch_thickness(ardesia_window)
    path = generate_rose_curve_path(bounding_box)
    simulate_drawing(path)
    
    (x, y, w, h) = bounding_box

def scenario_roundify(ardesia_window, bounding_box):
    clear_screen(ardesia_window)
    print("[*] Scenario Roundify")
    (x, y, w, h) = bounding_box
    switch_draw_mode(ardesia_window)
    
    push_button(ardesia_window, "Highlighter")
    
    select_color(ardesia_window, "Blue")
    switch_thickness(ardesia_window)
    path = generate_tilde_path(bounding_box)
    simulate_drawing(path)
    
    select_color(ardesia_window, "Yellow")
    switch_thickness(ardesia_window)
    path = generate_rose_curve_path(bounding_box)
    simulate_drawing(path)
    
    (x, y, w, h) = bounding_box
    
def scenario_rectify(ardesia_window, bounding_box):
    clear_screen(ardesia_window)
    print("[*] Scenario Rectify")
    (x, y, w, h) = bounding_box

    push_button(ardesia_window, "Pencil")
    
    switch_draw_mode(ardesia_window)
    
    select_color(ardesia_window, "Red")
    switch_thickness(ardesia_window)
    path = generate_tilde_path(bounding_box)
    simulate_drawing(path)
    
    select_color(ardesia_window, "Green")
    switch_thickness(ardesia_window)
    path = generate_rose_curve_path(bounding_box)
    simulate_drawing(path)
    
    (x, y, w, h) = bounding_box

def erase_scenario(ardesia_window, bounding_box):
    push_button(ardesia_window, "Eraser")
    switch_thickness(ardesia_window)
    
    path = generate_tilde_path(bounding_box)
    simulate_drawing(path)
    
    path = generate_rose_curve_path(bounding_box)
    simulate_drawing(path)
    
def arrow_scenario(ardesia_window, bounding_box):
    push_button(ardesia_window, "Arrow")
    
    select_color(ardesia_window, "Blue")
    path = generate_tilde_path(bounding_box)
    simulate_drawing(path)

def filler_scenario(ardesia_window, boundig_box):
    push_button(ardesia_window, "Highlighter")
    path, center = generate_ellipse_path(bounding_box)
    simulate_drawing(path)
    fill(ardesia_window, center[0], center[1])
    
def get_bounding_box_center(bounding_box):
    """
    Calculates the central point of a given bounding box.

    Args:
        bounding_box (tuple): A tuple of four integers (x, y, w, h) representing
            the geometry of the area, where (x, y) is the top-left
            corner and (w, h) are the width and height.

    Returns:
        tuple: A tuple (center_x, center_y) with the integer coordinates
               of the center point.
    """
    # Unpack the bounding box tuple
    x, y, w, h = bounding_box
    
    # Calculate the center coordinates
    center_x = x + w / 2
    center_y = y + h / 2
    
    # Return the center point as a tuple of integers
    return (int(center_x), int(center_y))
    
def pointer_scenario(ardesia_window, bounding_box):
    push_button(ardesia_window, "Default Pointer")
    center_point = get_bounding_box_center(bounding_box)
    rawinput.absoluteMotion(center_point[0], center_point[1]);
    rawinput.click(center_point[0], center_point[1], button=3)
    time.sleep(1)
    rawinput.click(center_point[0], center_point[1], button=3)

def text_scenario(ardesia_window, bounding_box):
    print("[*] Scenario Text")
    push_button(ardesia_window, "Text")
    center_point = get_bounding_box_center(bounding_box)
    delta = bounding_box[2] / 5
    initial_point = (center_point[0]-delta, center_point[1])
    rawinput.absoluteMotion(initial_point[0], center_point[1])
    #rawinput.click(initial_point[0], center_point[1])
    rawinput.press(initial_point[0], center_point[1])
    time.sleep(0.3)
    rawinput.release(initial_point[0], center_point[1])
    time.sleep(4)
    rawinput.typeText("I'm Ardesia, Hello World")
    rawinput.click(initial_point[0], center_point[1])

def color_selector_scenario(ardesia_window, bounding_box):
    print("[*] Scenario Color Selector")
    push_button(ardesia_window, "Select Color")
    color_selector_window=ardesia_window.parent.child(name="Changing color")
    #button_name='Custom color 5: Red 75%, Green 25%, Blue 25%, Alpha 63%'
    button_name='Custom color'
    push_button(color_selector_window, button_name)
    color_name_entry = color_selector_window.child(name="Color Name")
    color_name_entry.click()
    rawinput.keyCombo('<Ctrl>a')      # select all
    time.sleep(0.1)
    rawinput.keyCombo('BackSpace')    # delete
    rawinput.typeText("#FF00FF")
    rawinput.keyCombo("Return")
    time.sleep(1)
    push_button(color_selector_window, 'Select')
    path = generate_tilde_path(bounding_box)
    simulate_drawing(path)

def save_as_png(ardesia_window):
    push_button(ardesia_window, 'Screenshot')
    time.sleep(6)
    save_as_png_win_name="Save Screenshot as PNG"
    save_as_png_window=ardesia_window.parent.child(
        name=save_as_png_win_name
    )
    time.sleep(1)
    push_button(save_as_png_window, 'Save')
    time.sleep(1)
    
def add_to_pdf(ardesia_window, start=False):
    push_button(ardesia_window, "Save to PDF")
    if start:
        time.sleep(6)
        file_selector = ardesia_window.parent.child(name='Choose a file')
        time.sleep(3)
        push_button(file_selector, 'Save As')
        
def info_dialog(ardesia_window):
    push_button(ardesia_window, "About")
    about = ardesia_window.parent.child(name='About Ardesia')
    push_button(about, 'Close')
    
def quit(ardesia_window):
    push_button(ardesia_window, "Quit")
    
def change_background(ardesia_window):
    push_button(ardesia_window, "Background")
    background_selector_window = ardesia_window.parent.child(name='Backgrounds') 
    background_selector=background_selector_window.children[0]
    background_to_select = background_selector.children[-2]
    background_to_select.click()
    
def choose_font(ardesia_window):
    push_button(ardesia_window, "Fonts")
    time.sleep(10)
    font_selector=ardesia_window.parent.child(name='Choose Font')
    font_search=font_selector.child(name='Search')
    font_search.click()
    time.sleep(3)
    rawinput.typeText("DejaVu Serif Bold")
    rawinput.keyCombo("Return")

    
def run_all_scenarios(ardesia_window, bounding_box):
    color_selector_scenario(ardesia_window, bounding_box)
    save_as_png(ardesia_window)
    choose_font(ardesia_window)
    text_scenario(ardesia_window, bounding_box)
    add_to_pdf(ardesia_window, start=True)
    pointer_scenario(ardesia_window, bounding_box)
    add_to_pdf(ardesia_window)
    filler_scenario(ardesia_window, bounding_box)
    scenario_freehand(ardesia_window, bounding_box)
    add_to_pdf(ardesia_window)
    scenario_roundify(ardesia_window, bounding_box)
    scenario_rectify(ardesia_window, bounding_box)
    undo(ardesia_window)
    redo(ardesia_window)
    hide_unhide(ardesia_window)
    erase_scenario(ardesia_window, bounding_box)
    add_to_pdf(ardesia_window)
    arrow_scenario(ardesia_window, bounding_box)
    change_background(ardesia_window)
    info_dialog(ardesia_window)
    add_to_pdf(ardesia_window)
    quit(ardesia_window)
    print("[*] All scenarios executed.")

# === Main ===
if __name__ == "__main__":
    env = os.environ.copy()

    env["LANG"] = "en_US.UTF-8"
    env["LC_ALL"] = "en_US.UTF-8" # Aggiungere LC_ALL è più robusto

    ardesia_command = [
        "valgrind", "--track-origins=yes", "--leak-check=full", "--log-file=valgrind-report.txt", "./ardesia"
    ]
    #ardesia_command = [
    #    "./ardesia"
    #]
    
    print("Start Ardesia with Valgrind...")
    ardesia_process = subprocess.Popen(ardesia_command, preexec_fn=os.setsid, env=env)
    ardesia_window = None
    annotation_window = None
    print("Waiting Ardesia window...")
    for i in range(15):
        try:
            ardesia_windows = tree.root.application('ardesia')
            ardesia_window = ardesia_windows.child(name='Ardesia')
            annotation_window = ardesia_windows.child(name='Annotations')
            print("Found Ardesia window.")
            break
        except SearchError:
            print(f"Looking Ardesia window... (attempt {i+1})")
            time.sleep(1)
    bounding_box = annotation_window.extents
    run_all_scenarios(ardesia_window, bounding_box)
