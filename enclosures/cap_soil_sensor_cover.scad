/* Capacitive Soil Sensor case - Dave Hartburn April 2020
 *
 * Cover for a Capacitive Soil Moisture Sensor v1.2, intended to
 * protect from rain while outside.
 */
 
drawSensor=0;   // Set to 1 to show the sensor
drawMainCase=0; // Draw the main part of the casing
drawLid=1;

th=1.5;         // Thickness
marg=0.4;       // Space margin
caseH=15;       // Case height
caseL=50;       // Case length
bw=24.5;        // Board Width

if (drawSensor==1) {
    color([0.2,0.2,0.2]) drawSensor();
}

if (drawMainCase==1) {
    drawMainCase();
}

if (drawLid==1) {
    color([0,1,1]) drawLid();
}

module drawMainCase() {
   width=bw+(th+marg)*2;
   translate([-(th+marg),56,-th-marg]) {
        difference() {
            cube([width,caseL,caseH]);
            translate([th,0,th]) {
                cube([bw+(marg*2),caseL-th,caseH+10]);
            }
            // Slot for cable
            translate([(width-9.5)/2,caseL-7, -1]) {
                cube([9.5,4,th*5]);
            }
        }
        // Add 'holes'/lid support
        translate([th+marg,7,0]) {
            cylinder(d=2,h=5,$fn=64);
        }
        translate([bw+marg*2+th,7,0]) {
            cylinder(d=2,h=5,$fn=64);
        }
        // Top lid support
        translate([2,caseL-10,0]) {
            cube([2,10,caseH-(th*2+marg)]);
        }
        translate([bw+marg,caseL-10,0]) {
            cube([2,10,caseH-(th*2+marg)]);
        }
        // Add ridge for slots
        translate([0,0,caseH-th]) {
            difference() {
                cube([width,caseL,th]);
                translate([th*2,0,-1]) {
                    cube([width-4*th,caseL-2*th,th+2]);
                }
            }
        }
   }
}

module drawLid() {
    width=bw;
    length=caseL-th;
    height=caseH-(2*th+marg+1.3);
    translate([0,56,1.3]) {
        difference() {
            cube([width,length,height]);
            translate([0,th,0]) {
                cube([width,length-th,height-th]);
            }
        }
        // Add a 'handle'
        hw=5;
        translate([(width-hw)/2,-hw,0]) {
            cube([hw,hw,height]);
        }

    }
}

module drawSensor() {
    // Draw the sensor board
    
    // Main board
    difference() {
        hull() {
            cube([bw,85,1.3]);
            translate([11.5,-12,0]) cube([0.3,0.3,1.3]);
        }
        // Punch out two holes
        translate([0,63,-1]) {
            cylinder(d=5,h=3);
        }
        translate([bw,63,-1]) {
            cylinder(d=5,h=3);
        }
    }
    // Add connector
    translate([7,77,0]) {
        cube([10,10,8]);
    }
}