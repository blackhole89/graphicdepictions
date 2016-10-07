// gd node script N.v=not neigh
gamma(N).forEach(function (M) {
	if(!M.selected)
		N.v = 1-M.v;
});