// gd graph script multiplier
var p=0.7;

function pow(n,p) {
	var ret=1;
	for(var i=0;i<p;++i) 
		ret *= n;
	return ret;
}

nodes().forEach(function (N) {
	//if(!N.selected) return;

	var acc=0.0;
	nodes().forEach(function (M) {
		if(M.p<0.001) return;
		var diff=M.label^N.label;
		var pp = pow(p,5-countbits(diff))
   	         *pow(1-p,countbits(diff));
		acc += pp*M.v;
	});
	
	/*N2 = addNode(-0.1+N.pos.x,N.pos.y);
	N2.out=acc-0.5;
	N2.v=N2.p=0.0;
	N2.selected=true;
	N.selected=false;*/
	N.out = acc-0.5;
});