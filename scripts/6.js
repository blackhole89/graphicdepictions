// gd graph script select halfplane
nodes().forEach(function (N) {
	if(N.label & 1) N.selected=true;
	else N.selected=false;
});