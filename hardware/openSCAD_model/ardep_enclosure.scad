
$fn = 100;
// -----------------------
// User Parameters
// -----------------------
show_supports = true;
show_vents = true;
show_vga_cutout = true;
show_power_cutout = true;

outer_size = [72, 107, 8];
inner_size = [67, 102, 10];
outer_round_r = 1;

vga_pos = [-1.5, -51, 3];
vga_size = [32, 20, 10];

power_pos = [-40, -13.5, 3];
power_size = [15, 17, 10];

vent_x_range = [-35, 35];
vent_step = 10;
vent_size = [50, 0.5, 10];

support_points = [
    [-28.5, -47],
    [-28.5,  45],
    [ 23.5,  45],
    [ 23.5, -47]
];

// -----------------------
// Helper Modules
// -----------------------
module rounded_box(size_xyz, r) {
    minkowski() {
        cube(size_xyz, center = true);
        sphere(r);
    }
}
module support_post(xy, base_h = 2, base_r = 3, pin_h = 4, pin_r = 1.5) {
    translate([xy[0], xy[1], -2]) cylinder(h = base_h, r = base_r, center = true);
    translate([xy[0], xy[1], -1]) cylinder(h = pin_h, r = pin_r, center = true);
}

module cutout_rounded(size_xyz, pos_xyz, r) {
    translate(pos_xyz) rounded_box(size_xyz, r);
}

module vents() {
    for (x = [vent_x_range[0] : vent_step : vent_x_range[1]]) {
        translate([0, x, 0]) rounded_box(vent_size, outer_round_r);
    }
}

module case_body() {
    difference() {
        rounded_box(outer_size, outer_round_r);
        translate([0, 0, 3]) cube(inner_size, center = true);
        translate([0, 0, 3]) cube([100, 150, 5], center = true);

        if (show_vga_cutout) cutout_rounded(vga_size, vga_pos, outer_round_r);
        if (show_power_cutout) cutout_rounded(power_size, power_pos, outer_round_r);
        if (show_vents) vents();
    }
}

// -----------------------
// Final Model
// -----------------------
case_body();

if (show_supports) {
    for (pt = support_points) {
        support_post(pt);
    }
}

