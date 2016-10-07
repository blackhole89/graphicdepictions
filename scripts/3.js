// gd node script N.p calc
function pow(n,p) {
	var ret=1;
	for(var i=0;i<p;++i) 
		ret *= n;
	return ret;
}

var p = 0.6;
var target=0;

var diff=target^N.label;
N.p = pow(p,5-countbits(diff))
     *pow(1-p,countbits(diff));
