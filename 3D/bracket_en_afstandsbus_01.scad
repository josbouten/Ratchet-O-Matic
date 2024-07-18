// Steunbeugel voor de PCBs van de Ratchet-O-Matic.abs

$fn = 160;

as_lengte = 4; // mm.

opzet_blok_hoogte = 15;
opzet_blok_x = 7; 
opzet_blok_y = 10;
basis_height = 2;

module as() {
    diameter = 3.0;
    cylinder(h=as_lengte, r = diameter / 2, center=true);
}

module basis_links() {
    x = 12;
    y = 10;   
    cube([x, y, basis_height], center = true);
}


module opzet_blok() {
    cube([opzet_blok_x, opzet_blok_y, opzet_blok_hoogte], center = true);
}

module boorgat_links() {
    #translate([3, 0, 0]) rotate([0, 90, 0]) cylinder(h=opzet_blok_x, r=3.5 / 2.0, center = true);
}

module blokje_links() {
    translate([0, 0, as_lengte / 2]) as();
    translate([3.5, 0, as_lengte + basis_height / 2]) basis_links();
    x_verplaatsing = 6;
    translate([x_verplaatsing, 0, as_lengte + basis_height + opzet_blok_hoogte / 2]) opzet_blok();
}

module bracket() {
    difference() {
        blokje_links();
        boorgat_hoogte = as_lengte + basis_height + opzet_blok_hoogte - 3;
        echo("boorgat hoogte:", boorgat_hoogte);
        translate([3, 0, boorgat_hoogte]) boorgat_links();
    }
}

module afstandsbus() {
    difference() {
        cylinder(8.4, 6 / 2, 6 / 2);
        cylinder(8.4, 3.5 / 2, 3.5 / 2);
    }
}

module alles(a = 0, b = 0) {
    if (a == 1) {
        translate([20, 0, 0]) bracket();
    }
    if (b == 1) {
        afstandsbus();
    }
}

alles(1, 1);