#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

int main(void)
{

FILE *f = fopen("map.html", "w");
	if (f == NULL) {
		printf("Unable to open walk.html\n");
		exit(1);
	}

const char *header = "<html><head>\n<style>\n#map {height: 100%;}\nhtml,"
		"body {height: 100%; margin: 0; padding: 0;}\n</style>\n<script>\n"
		"function initMap() {\n";

const char *prepmap = "var edges = [v1,v2,v3,v4,v5];\n"
	"var map = new google.maps.Map(document.getElementById('map'), {\n"
	"zoom: 4, center: v1});\nvar path = new google.maps.Polyline({"
	"path: edges, geodesic: true, strokeColor: '#FF0000', strokeOpacity:"
	"1.0, strokeWeight: 2});\n\npath.setMap(map);\n";

const char *mapdone = "<!-- Define the function vars -->\n"
	"var if1 = new google.maps.InfoWindow({\n  content: v1info\n});\n"
	"var if2 = new google.maps.InfoWindow({\n  content: v2info\n});\n"
	"var if3 = new google.maps.InfoWindow({\n  content: v3info\n});\n"
	"var if4 = new google.maps.InfoWindow({\n  content: v4info\n});\n"
	"var if5 = new google.maps.InfoWindow({\n  content: v5info\n});\n"
	"var m1 = new google.maps.Marker({ position: v1, map: map, title: 'v1' });\n"
	"var m2 = new google.maps.Marker({ position: v2, map: map, title: 'v2' });\n"
	"var m3 = new google.maps.Marker({ position: v3, map: map, title: 'v3' });\n"
	"var m4 = new google.maps.Marker({ position: v4, map: map, title: 'v4' });\n"
	"var m5 = new google.maps.Marker({ position: v5, map: map, title: 'v5' });\n"
	"m1.addListener('click', function() { if1.open(map, m1);});\n"
	"m2.addListener('click', function() { if2.open(map, m2);});\n"
	"m3.addListener('click', function() { if3.open(map, m3);});\n"
	"m4.addListener('click', function() { if4.open(map, m4);});\n"
	"m5.addListener('click', function() { if5.open(map, m5);});\n}\n";

const char *footer = "</script></head><body><div id='map'></div><script async "
			"defer src='https://maps.googleapis.com/maps/api/js?key"
			"=AIzaSyDHzAToNUhu1VMLVtOtNqTAW2OYehK70m8&callback=init"
			"Map'></script></body></html>";

/* Generate the HTML */
fprintf(f, "%s\n", header);
fprintf(f, "%s\n", prepmap);
fprintf(f, "%s\n", mapdone);
fprintf(f, "%s\n", footer);

fclose(f);

return 0;
}
