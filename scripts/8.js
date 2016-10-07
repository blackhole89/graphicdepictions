// gd graph script sum over nodes
O = addNode(-0.2,0);

var sum = 0.0;

nodes().forEach(function (N) {
	if(N.p<0.0001) return undefined;
	
	sum += N.out<0?-N.out:N.out;

	N.selected=false;
});

O.out = sum;
O.selected=true;