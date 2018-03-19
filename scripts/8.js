// gd graph script sum over nodes
var sum = 0.0;

nodes().forEach(function (N) {
	if(N.p<0.0001) return undefined;
	
	sum += N.out<0?-N.out:N.out;

	N.selected=false;
});

println("Sum: "+sum);