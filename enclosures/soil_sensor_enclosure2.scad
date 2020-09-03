/* Soil sensor enclosure - To hold solar panel
 * and BME280.
 * Note: Originally taken from a much larger enclosure, which is why there is not
 * so much venting.
 * Dave Hartburn - July 2020
 */
use <library/ventedPanel.scad>;
use <library/prisms.scad>;

// Solar panel (plus a small margin)
solw=117;
sold=87;
solh=4;

// Enclosure parameters
solbord=4;  // Amount of border around solar panel
th=2.4;   // Thickness of the sides
w=solw+solbord*2;     // Width, depth and height of main enclosure
d=sold+solbord*2;
h=30;           // Height of front/main usable part
roofAngle=20;   // Angle of the roof
roofoh=7;       // Roof overhang
ventbh=12;       // Vent border horizontan
ventbv=8;        // Vent border vertical
ventang=45;      // Angle of vents
// Use a nylon screw as a peg to hold on the front
fpegd=3.1;  // Diameter of peg hole
fpegCoords=[        // Array of coordinates for peg holes, 2 may do
    [-8,3.5,15],
    [w-8,3.5,h-15]
    ];

// Hangers, top and bottom for screwing to something
hangw=35;   // Width
hangLowOff=20;   // How much below it hangs
hangBarLen=100; // Imagine the hangers as a bar going through, this is how long it is
                // Should really calculate it to make this all nice and self-adjusting
                // Adjust manually
hangHole=4; // Mount hole diameter

// What to show?
showMain=0;     // Main part of the enclosure
showSides=1;    // Cut out the sides, useful for positioning components
showHangers=1;  // Hangers for connecting mounting to something
showFront=1;

// Components - not for printing!
showSolarPanel=1;   // Not implemented

$fn=256;

if(showMain==1) {
    drawMainEnclosure();
}
if(showFront==1) {
    color([0.5,1,0.5]) drawFront();
}


// **************************
module drawMainEnclosure() {
    // Main enclosure is made up of sides rather than cutting away
    // from a shape
    
    // Base
    difference() {
        cube([w,d,th]);
        // Hole for wire assembly
        whw=10;
        whd=5;
        translate([w/2-(whw/2),d*0.7,-th*2]) {
            cube([10,whd,th*4]);
        }
    }
    
    // Back
    translate([0,d-th,th]) {
        cube([w,th,h-th]);
        if(showHangers==1) {
            // Add hangers 
            translate([w/2,th,-hangLowOff]) {
                hangerBar();
            }
        }
        // Add sensor holder
        translate([0,th,0]) {
            sensorHolder();
        }
    }
    if(showSides==1) {
        difference() {
            union() {
                // Left side
                translate([0,d,0]) {
                    rotate([0,0,-90]) {
                        ventedPanel(d,h,th,ventbh,ventbv,1.2,2,ventang);
                    }
                }
                // Door sliders
                translate([w,0,0]) {
                    rotate([0,0,90]) {
                        ventedPanel(d,h,th,ventbh,ventbv,1.2,2,ventang);
                    }
                }
            }
            fpegHoles();
        }
    }
    // Roof
    translate([-roofoh,-roofoh,h]) {
        difference() {
            raPrismUpright(w+roofoh*2,d+roofoh,tan(roofAngle)*(d+roofoh));
            // Hollow out
            translate([roofoh+th,roofoh+solh+th,-0.001]) {
                raPrismUpright(w-th*2,d-solh-roofoh,tan(roofAngle)*(d-th-solh-roofoh));
                
            }
            // Solar panel
            rotate([roofAngle,0,0]) {
                translate([solbord+roofoh,solbord+roofoh,-solh+1.5]) {
                    cube([solw,sold,solh]);
                }
                // Cut out for wires (fixed coords)
                translate([solbord+roofoh+48,solbord+roofoh+28,-10]) {
                    cube([20,11,20]);
                }
            }
        }
    }


}

module drawFront() {
    difference() {
        translate([-th,-th,0]) {
            ventedPanel(w+th*2,h,th,ventbh,ventbv+4,1.2,2,ventang);
            // Side grips
            cube([th,10,h]);
            translate([w+th,0,0]) cube([th,10,h]);
            translate([th*2+0.3,th,th*2]) {
                cube([th,10-th,h-th*4]);
            }
            translate([w-(th+0.3),th,th*2]) {
                cube([th,10-th,h-th*4]);
            }
        }
        fpegHoles();
    }
}

// Remove front peg holes
module fpegHoles() {
    for(i=fpegCoords) {
        translate(i) {
            rotate([0,90,0]) {
                cylinder(h=20,d=fpegd);
            }
        }
    }
}

module hangerBar() {
    // Draws a bar to hang the unit on
    rotate([90,0,0]) {
        difference() {
            hull() {
                cylinder(h=th,d=hangw);
                translate([0,hangBarLen,0]) {
                    cylinder(h=th,d=hangw);
                }
            }
        
            // Poke through screw holes
            translate([0,0,-th]) {
                cylinder(h=th*3,d=hangHole);
            }
            translate([0,hangBarLen,-th]) {
                cylinder(h=th*3,d=hangHole);
            }
        }
    }
}

module sensorHolder() {
    // Can hold the sensor just by the wires
    holdW=4;
    holdD=20;
    holdH=10;
    translate([40,-(holdD+th+5),15]) {
        difference() {
            cube([holdW+th*2,holdD+5,holdH]);
            translate([th,th,-1]) {
                cube([holdW,holdD-th,holdH+2]);
            }
        }
    }
}