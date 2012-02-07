module corner(x, y, z, diameter, faces, thickness_over_width ){
	translate([x, y, z])  scale([1,1,thickness_over_width]) sphere( r = diameter/2, $fn = faces );
}

module tube(x1, y1, z1, x2, y2, z2, diameter1, diameter2, faces, thickness_over_width)
{
	length = sqrt( pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2) );
	alpha = ( (y2 - y1 != 0) ? atan( (z2 - z1) / (y2 - y1) ) : 0 );
 	beta = 90 - ( (x2 - x1 != 0) ? atan( (z2 - z1) / (x2 - x1) ) :  0 );
	gamma =  ( (x2 - x1 != 0) ? atan( (y2 - y1) / (x2 - x1) ) : ( (y2 - y1 >= 0) ? 90 : -90 ) ) + ( (x2 - x1 >= 0) ? 0 : -180 );
	// echo(Length = length, Alpha = alpha, Beta = beta, Gamma = gamma);	
	translate([x1, y1, z1])
	rotate([ 0, beta, gamma])
		scale([thickness_over_width,1,1])
			rotate([0,0,90]) cylinder(h = length, r1 = diameter1/2, r2 = diameter2/2, center = false, $fn = faces );
}
// use min and max to see individual layers 
min = 0;
max = 174;
// triangles(min,max);
outlines(min,max);
// infills(min,max);

stl_color = [0,1,0, 0.025];
module out_line(x1, y1, z1, x2, y2, z2)
{
    tube(x1, y1, z1, x2, y2, z2, diameter1=0.4, diameter2=0, faces=4, thickness_over_width=1);
}

module extrusion(x1, y1, z1, x2, y2, z2)
{
    d = 0.350000;
    f = 6;
    t =  0.600000;
    corner(x1,y1,z1, diameter=d, faces=f, thickness_over_width =t );
    tube(x1, y1, z1, x2, y2, z2, diameter1=d, diameter2=d, faces=f, thickness_over_width=t);
}

module outline(points, paths)
{
    for (p= paths)
    {
       out_line(points[p[0]][0],points[p[0]][1],points[p[0]][2],points[p[1]][0],points[p[1]][1],points[p[1]][2] );
    }
}


module infill(points, paths)
{
     for (p= paths)
    {
        extrusion(points[p[0]][0],points[p[0]][1],points[p[0]][2], points[p[1]][0],points[p[1]][1],points[p[1]][2] );
    }
}


module outline_segments(segments)
{
    for(seg = segments)
    {
        out_line(seg[0][0], seg[0][1], seg[0][2], seg[1][0], seg[1][1], seg[1][2]);
    }
}

module infill_segments(segments)
{
    for(seg = segments)
    {
        extrusion(seg[0][0], seg[0][1], seg[0][2], seg[1][0], seg[1][1], seg[1][2]);
   }
}
