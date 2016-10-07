 /*
  * graphic depictions, a visual workbench for graphs 
  * 
  * Copyright (C) 2016 Matvey Soloviev
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

#include "export.h"

void RGBtoYUV( float r, float g, float b, float *y, float *u, float *v )
{
	*y = 0.2126*r + 0.7152*g + 0.0722*b;
	*u = -0.09991*r - 0.33609*g + 0.436*b;
	*v = 0.615*r  - 0.55861*g - 0.05639*b;
}

void YUVtoRGB( float *r, float *g, float *b, float y, float u, float v )
{
	*r = y + 1.28033*v;
	*g = y - 0.21482*u - 0.38059*v;
	*b = y + 2.12798*u;
}

void ExportTikz(CSState *st, char *filename)
{
    FILE *fl=fopen(filename,"w");
    if(!fl) return;

    fprintf(fl,"\\begin{tikzpicture}[x=800pt,y=800pt]\n");

    /* style preamble */
    fprintf(fl,"\\tikzstyle{n} = [fill=black, inner sep=2pt, circle,font=\\small]\n");
    fprintf(fl,"\\tikzstyle{nl} = [font=\\tiny]\n");
    fprintf(fl,"\\definecolor{clredge}{rgb}{%.2f,%.2f,%.2f}\n",s.e->graphics.config.edge[0],s.e->graphics.config.edge[1],s.e->graphics.config.edge[2]);
    fprintf(fl,"\\tikzstyle{e} = [draw=clredge]\n");

    /* label styles */
    for(int y=0;y<3;++y) for(int x=0;x<3;++x) {
        float r,g,b,Y,U,V;
        r=::s.e->graphics.nodecol[y*3+x][0];
        g=::s.e->graphics.nodecol[y*3+x][1];
        b=::s.e->graphics.nodecol[y*3+x][2];
        RGBtoYUV(r,g,b,&Y,&U,&V);
        Y = 1.0f-Y;
        YUVtoRGB(&r,&g,&b,Y,U,V);
        /* make sure we are in [0.0,1.0] */
        float over = r; if(g>over) over=g; if(b>over) over=b;
        float under = r; if(g<under) under=g; if(b<under) under=b;
        float adjust;
        if(over>1.0f) adjust=1.0f-over;
        else if(under<0.0f) adjust=-under;
        else adjust=0.0f;

        fprintf(fl,"\\definecolor{clr%d%d}{rgb}{%.2f,%.2f,%.2f}\n",x,y,abs(r+adjust),abs(g+adjust),abs(b+adjust));
    }

    /* nodes */
    std::map<CSState::CSNode *,int> nodes2id; int id=0;
    for(auto i=st->nodes.begin(); i!=st->nodes.end(); ++i) {
        float sx,sy;
        s.e->graphics.SpaceToScreen((*i)->pos[0],(*i)->pos[1],(*i)->pos[2],sx,sy);
        /* scale relative to window size */
        sx=(sx/s.wnd->w)-0.5f;
        sy=((sy+(s.wnd->w-s.wnd->h)/2)/s.wnd->w)-0.5f;
        /* flip y for tikz */
        sy*=-1;

        fprintf(fl,"\\coordinate (n%d) at (%.3f,%.3f) {};\n",id,sx,sy);

        nodes2id[*i]=id++;
    }

    /* edges */
    for(auto i=st->edges.begin(); i!=st->edges.end(); ++i) {
        fprintf(fl,"\\draw[e] (n%d) -- (n%d);\n",nodes2id[(*i)->n1],nodes2id[(*i)->n2]);
    }

    /* node dots and labels */
    for(auto i=st->nodes.begin(); i!=st->nodes.end(); ++i) {
        fprintf(fl,"\\node[n] (dot) at (n%d) {};\n",nodes2id[*i]);

        char *anchors[3][3] = { {",anchor=south east",",anchor=south",",anchor=south west"} , {",anchor=east","",",anchor=west"},  {",anchor=north east", ",anchor=north", ",anchor=north west"}};
        for(int y=0;y<3;++y) for(int x=0;x<3;++x) {
            if( (*i)->a.count( s.e->graphics.nodelook[y*3+x] ) ) {
                CSState::CSAttr *a = &(*i)->a[s.e->graphics.nodelook[y*3+x]];
                char buf[32]={0};
                switch(a->type) {
                case CSState::CSAttr::TY_INT:
                    sprintf(buf,"%d",a->data.d_int);
                    break;
                case CSState::CSAttr::TY_BITS:
                    int b;
                    for(b=0;b<=floor(0.01f+log(a->data.d_bits)/log(2.0f));++b) {
                        buf[b]=(a->data.d_bits&(1<<b))?'1':'0';
                    }
                    buf[b]=0;
                    if(!b) {
                        buf[b]='0';
                        buf[b+1]=0;
                    }
                    break;
                case CSState::CSAttr::TY_FLOAT:
                    sprintf(buf,"%.3f",a->data.d_float);
                    break;
                }

                fprintf(fl,"\\draw (n%d) node[color=clr%d%d,nl%s] {$%s$};\n", nodes2id[*i], x,y, anchors[y][x], buf);
            }
        }


    }

    fprintf(fl,"\\end{tikzpicture}\n");


    fclose(fl);}
