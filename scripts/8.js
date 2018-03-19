// gd graph script Degree statistics
/* save stats globally */
stats = {};

nodes().forEach(function(N) {
	var d = gamma(N).length;
	if(!stats[d]) stats[d]=0;
	++stats[d];
});

println("== Graph degree statistics ==");
for(var d in stats) {
	println("Degree "+d+": "+stats[d]);
}
