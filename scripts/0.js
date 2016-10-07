// gd graph script random regular
var n = 20;
var d = 6;

var ns = [];
var ids = [];

for(var i=0;i<n;++i) {
	ns.push(addNode( 0.1*(rand()%10), 0.1*(rand()%10) ));
	ns[i].selected = true;
	ids.push(i);
}

// make random permutation with all cycles
// having at least length 3
function shufflebigcycles(a) {
	var i, j, temp, ok;
	do {
		ok=true;
		i=a.length;
		while(i) {
			j=rand()%i;
			--i;
			temp=a[i];
			a[i]=a[j];
			a[j]=temp;
		}
		for(var k=0;k<a.length;++k) {
			if(a[a[k]]==k) ok=false;
		}
	} while(!ok);
}

// invert permutation
function invert(a) {
	r = a.slice();
	for(var i=0;i<a.length;++i) {
		r[a[i]]=i;
	}
	return r;
}

var idcopies = [];

for(var j=0;j<(d/2);++j) {
	idcopies.push(ids.slice());
	var dups;
	do {
		shufflebigcycles(idcopies[2*j]);
		dups = false;
		for(var k=0;k<2*j;++k) {
			for(var l=0;l<n;++l) {
				if(idcopies[k][l]==idcopies[2*j][l])
					dups = true;
			}
		}
	} while(dups);
	idcopies.push(invert(idcopies[2*j]));
	for(var i=0;i<n;++i) {
		addEdge(ns[i],ns[idcopies[2*j][i]]);
		
	}
}