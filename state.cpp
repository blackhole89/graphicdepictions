 /*
  * graphic depictions, a visual workbench for graphs
  *
  * Copyright (C) 2016-2018 Matvey Soloviev
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

#include "stdafx.h"

#ifndef WIN32
//#include <gtk.h>
#else
#endif

#include "v8/v8-profiler.h"

/* deferred load */
CSState::CSAttr CSState::CSAttr::FromStringDeferred(char *j)
{
    CSAttr ret;
    ret.st=j;
    return ret;
}
void CSState::CSAttr::Compile()
{
    j_data = v8::Persistent<v8::Value>::New(s.e->FromJSON((char*)st.c_str()));
    j_data.SetWrapperClassId(1);
    st="";
}

CSState::CSAttr::CSAttr(char *j)
{
    j_data = v8::Persistent<v8::Value>::New(s.e->FromJSON(j));
//    printf("Count: %d\n",v8::HeapProfiler::GetPersistentHandleCount());
}

CSState::CSAttr::CSAttr()
{}

CSState::CSAttr &CSState::CSAttr::operator=(CSAttr &&other)
{
    j_data.Dispose();
    j_data = other.j_data;
    st = other.st;
    other.j_data = v8::Persistent<v8::Value>();
    return *this;
}

CSState::CSAttr &CSState::CSAttr::operator=(const CSAttr &other)
{
    printf("copy (%llX <- %llX)\n",this,&other);
    return *this;
}

CSState::CSAttr::CSAttr(const CSAttr &other)
{
    operator=(other);
}

CSState::CSAttr::~CSAttr()
{
//    printf("destroy pcell %X: %s\n",identity,*v8::String::Utf8Value(j_data));
    j_data.Dispose();
//    v8::HeapStatistics stats;
//    v8::V8::GetHeapStatistics(&stats);
//    printf("heap:  %d/%d\n", stats.used_heap_size(), stats.total_heap_size());
}

std::string CSState::CSAttr::PrettyPrint()
{
    return std::string(*v8::String::Utf8Value(j_data));
}

std::string CSState::CSAttr::ToString()
{
    std::string str = s.e->ToJSON(j_data);
    char buf[32];
    sprintf(buf,"j%d,",str.length());
    return buf+str;
}

CSState::CSAttr CSState::CSAttr::FromString(char *buf)
{
    return CSAttr(buf);
}

double CSState::CSAttr::ArrayGet(int i)
{
    if(!j_data->IsArray()) return 0.0;
    if(!j_data.As<v8::Array>()->Get(i)->IsNumber()) return 0.0;    
    return j_data.As<v8::Array>()->Get(i)->NumberValue();   
}

CSState::CSNode *CSState::AddNode(float x, float y, float z)
{
    CSNode *n = new CSNode;
    n->selected=false;
    n->pos[0]=x;
    n->pos[1]=y;
    n->pos[2]=z;

    nodes.insert(n);

    return n;}

void CSState::DelNode(CSNode *n)
{
    /* remove references inside JS */
    s.e->DeepWipe(n);

    /* remove edges touching it */
    for(auto i=edges.begin(); i!=edges.end(); ) {
        if( (*i)->n1 == n || (*i)->n2 == n) {
            auto j=i;
            ++i;
            DelEdge(*j);
        } else ++i;
    }

    nodes.erase(n);

    for(int i=0;i<NSELGROUPS;++i) {
        selgroups[i].erase(n);
    }

    delete n;
}

void CSState::DelEdge(CSEdge *e)
{
    s.e->DeepWipe(e);

    e->n1->adje.erase(e);
    e->n2->adje.erase(e);
    e->n1->adj.erase(e->n2);
    e->n2->adj.erase(e->n1);

    edges.erase(e);
    delete e;
}

void CSState::DelEdge(CSNode *n1, CSNode *n2)
{
    for(auto i=edges.begin(); i!=edges.end(); ++i ) {
        if( (*i)->n1 == n1 && (*i)->n2 == n2 ) {
            DelEdge(*i);
            return;
        }
        if( (*i)->n1 == n2 && (*i)->n2 == n1 ) {
            DelEdge(*i);
            return;
        }
    }
}

CSState::CSEdge *CSState::GetEdge(CSNode *n1, CSNode *n2)
{
    for(auto i=n1->adje.begin();i!=n1->adje.end();++i) {
        if( (*i)->n1 == n2 || (*i) ->n2 == n2 ) return *i;
    }
    return NULL;
}

CSState::CSEdge *CSState::AddEdge(CSState::CSNode *n1, CSState::CSNode *n2)
{
    if(n1==n2) return NULL;

    if(n1->adj.count(n2)) {
        for(auto i=n1->adje.begin();i!=n1->adje.end();++i) {
            if( (*i)->n1 == n2 || (*i) ->n2 == n2 ) return *i;
        }
        return NULL;
    }

    CSEdge *e = new CSEdge;
    e->selected=false;
    e->n1=n1;
    e->n2=n2;

    edges.insert(e);
    n1->adje.insert(e);
    n2->adje.insert(e);
    n1->adj.insert(n2);
    n2->adj.insert(n1);

    return e;
}

/* manual modelling */
void CSState::DuplicateSelection()
{
    std::map<CSNode*,CSNode*> old2new;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if( (*i)->selected ) {
            // create new node
            CSNode *n1 = AddNode( (*i)->pos[0], (*i)->pos[1], (*i)->pos[2] );
            old2new[*i]=n1;

            // connect duplicates of old nodes to it
            for(auto j=nodes.begin(); j!=i; ++j) {
                if( (*j)->selected ) {
                    if( (*j)->adj.count(*i) )
                        AddEdge(n1,old2new[*j]);
                }
            }
        }
    }
    for(auto i=old2new.begin(); i!=old2new.end(); ++i) {
        i->second->selected=true;
        i->first->selected=false;    }
}

void CSState::ExtrudeSelection()
{
    std::map<CSNode*,CSNode*> old2new;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if( (*i)->selected ) {
            // create new node
            CSNode *n1 = AddNode( (*i)->pos[0], (*i)->pos[1], (*i)->pos[2] );
            old2new[*i]=n1;

            // connect new to old
            AddEdge(*i,n1);

            // connect duplicates of old nodes to it
            for(auto j=nodes.begin(); j!=i; ++j) {
                if( (*j)->selected ) {
                    if( (*j)->adj.count(*i) )
                        AddEdge(n1,old2new[*j]);
                }
            }
        }
    }
    for(auto i=old2new.begin(); i!=old2new.end(); ++i) {
        i->second->selected=true;
        i->first->selected=false;    }
}

/* manipulate selection */
void CSState::GrowSelection()
{
    std::queue<CSNode*> nq;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if ( (*i)->selected ) {
            nq.push(*i);
        }
    }
    while(!nq.empty()) {
        CSNode* t = nq.front();
        nq.pop();

        for(auto j=t->adj.begin(); j!=t->adj.end(); ++j) {
            (*j)->selected=true;
        }
    }

    nselected=0;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if( (*i)->selected ) ++nselected;
    }
}

void CSState::ShrinkSelection()
{
    std::queue<CSNode*> nq;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if ( !(*i)->selected ) {
            nq.push(*i);
        }
    }
    while(!nq.empty()) {
        CSNode* t = nq.front();
        nq.pop();

        for(auto j=t->adj.begin(); j!=t->adj.end(); ++j) {
            (*j)->selected=false;
        }
    }

    nselected=0;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if( (*i)->selected ) ++nselected;
    }
}

void CSState::SelectConnected()
{
    std::queue<CSNode*> nq;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if ( (*i)->selected ) {
            nq.push(*i);
        }
    }
    while(!nq.empty()) {
        CSNode* t = nq.front();
        nq.pop();

        for(auto j=t->adj.begin(); j!=t->adj.end(); ++j) {
            if(!(*j)->selected ) nq.push(*j);
            (*j)->selected=true;
        }
    }

    nselected=0;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if( (*i)->selected ) ++nselected;
    }
}

void CSState::InvertSelection()
{
    nselected = nodes.size()-nselected;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        (*i)->selected = !(*i)->selected;
    }}

/* selection groups */
void CSState::LoadSelectionGroup(int id)
{
    nselected = selgroups[id].size();
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        (*i)->selected = selgroups[id].count(*i);
    }
}

void CSState::SaveSelectionGroup(int id)
{
    selgroups[id].clear();
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if((*i)->selected) selgroups[id].insert(*i);
    }
}

/* advanced funs */
void CSState::MkDiscreteCube(int dim)
{
    CSNode **nodes = new CSNode*[1<<dim];

    for(int i=0;i<1<<dim;++i) {
        float x,y,z;
        x=(bool)(i&1);
        y=(bool)(i&2);
        z=(bool)(i&4);
        if(!s.e->use_3d) {
            y+=0.5*z;
            x+=0.5*z;
            z=0;
        }
        float coord_order2[2][2] = { {1,0.25}, {0.25,1} };
        float coord_order3[3][3] = { {1,0.25,0.25}, {0.25,1,0.25}, {0.25,0.25,1} };
        for(int d=3;d<dim;++d) {
            float offs = 1.2*(1<<(d/3))* ((bool) (i&(1<<d)));
            if(s.e->use_3d) {
                x += offs*coord_order3[d%3][0];
                y -= offs*coord_order3[d%3][1];
                z += offs*coord_order3[d%3][2];
            } else {
                x += offs*coord_order3[d%2][0];
                y -= offs*coord_order3[d%2][1];
            }
        }
        nodes[i] = AddNode( x*0.1, y*0.1, z*0.1 );
        if(!nodes[i]->selected) ++nselected;
        nodes[i]->selected=true;
        char buf[12];
        sprintf(buf,"%d",i);
        nodes[i]->a["label"] = CSAttr(buf);
    }

    for(int i=0;i<1<<dim;++i) {
        for(int j=0;j<dim;++j) {
            if( (i|(1<<j)) != i)
                AddEdge(nodes[i], nodes[i|(1<<j)]);
        }
    }

    delete nodes;
}

void CSState::MkGnp(int n, float p)
{
    CSNode **nodes = new CSNode*[n];

    for(int i=0;i<n;++i) {
        float x,y,z;
        x=-0.5+ (rand()%100) * 0.01;
        y=-0.5+ (rand()%100) * 0.01;
        if(s.e->use_3d) z=-0.5+ (rand()%100) * 0.01;
        else z=0;
        nodes[i]=AddNode(x,y,z);

        for(int j=0;j<i;++j) {
            if( ((rand()%10000)*0.0001)<p ) AddEdge(nodes[i],nodes[j]);
        }
    }

    delete nodes;
}

void CSState::OrderFromSelection()
{
    std::queue<CSNode *> nq;

    float xpos=0.0f;
    float ypos=0.0f;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if( (*i)->selected ) nq.push( *i);
    }
    nq.push(NULL);

    std::set<CSNode*> seen;
    while(!nq.empty()) {
        CSNode *t = nq.front();
        nq.pop();
        if(!t) {
            xpos += 0.1f;
            ypos = 0.0f;
            if(nq.size()) nq.push(NULL);
            continue;
        }
        if(seen.count(t)) continue;
        seen.insert(t);
        t->pos[0]=xpos;
        t->pos[1]=ypos;
        t->pos[2]=0;
        ypos += 0.05f;
        for(auto i=t->adj.begin(); i!=t->adj.end(); ++i) {
            nq.push( *i);
        }    }
}


bool CSState::RelaxationStep(float tlength)
{
    float dirs[7][3] = { {0,0,0}, {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
    float factor=0.002+0.0002*(rand()%10);
    float sqrt_tlength=sqrt(tlength);
    bool done = false;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        if ( (*i)->selected ) {
            float minenergy=9999999.0f;
            int mindir;

            std::vector<CSNode*> nonadj;
            std::set_difference(nodes.begin(),nodes.end(),(*i)->adj.begin(),(*i)->adj.end(),
                std::back_inserter(nonadj));
            //std::remove_if(nonadj.begin(), nonadj.end(), [](CSNode *a) { return !a->selected; });

            // perturb
            dirs[0][0] = ((rand()%11)-5)*0.01;
            dirs[0][1] = ((rand()%11)-5)*0.01;
            dirs[0][2] = ((rand()%11)-5)*0.01;

            for(int d=0;d<(5+2*(s.e->use_3d));++d) {
                float energy=0;
                for(auto j=(*i)->adj.begin(); j!=(*i)->adj.end(); ++j) {
                    float dx = (*i)->pos[0]+factor*dirs[d][0] - (*j)->pos[0];
                    float dy = (*i)->pos[1]+factor*dirs[d][1] - (*j)->pos[1];
                    float dz = (*i)->pos[2]+factor*dirs[d][2] - (*j)->pos[2];
                    float length_dev = (sqrt(dx*dx + dy*dy + dz*dz)-tlength);
                    energy += length_dev*length_dev;
                }

                for(auto j=nonadj.begin(); j!=nonadj.end(); ++j) {
                    float dx = (*i)->pos[0]+factor*dirs[d][0] - (*j)->pos[0];
                    float dy = (*i)->pos[1]+factor*dirs[d][1] - (*j)->pos[1];
                    float dz = (*i)->pos[2]+factor*dirs[d][2] - (*j)->pos[2];
                    float sqdist = sqrt( dx*dx + dy*dy + dz*dz);
                    if(sqdist>2*tlength) sqdist=2*tlength;
                    //if(sqdist > tlength) sqdist=tlength;
                    energy += tlength*tlength/(sqdist+tlength) - tlength/4.0; //4*( (*i)->adj.size()/(float)nonadj.size() )* (tlength-sqdist)*(tlength-sqdist);
                }

                if(energy<minenergy) {
                    minenergy=energy;
                    mindir=d;
                }
            }
            done |= mindir;
            (*i)->pos[0] += factor*dirs[mindir][0];
            (*i)->pos[1] += factor*dirs[mindir][1];
            (*i)->pos[2] += factor*dirs[mindir][2];
        }
    }
    return done;
}

void CSState::RoundPositions(float gridsize)
{
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        for(int j=0;j<3;++j) {
            (*i)->pos[j] = gridsize*round( (*i)->pos[j]/gridsize );
        }
    }
}

/* load/save */
void CSState::Clear()
{
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        delete (*i);
    }
    nodes.clear();
    for(auto i=edges.begin(); i!=edges.end(); ++i) {
        delete (*i);
    }
    edges.clear();
    nselected=0;
    for(int i=0;i<NSELGROUPS;++i) {
        selgroups[i].clear();
    }
    last_filename[0]=0;
    scripts.clear();

    /* reset global script context */
    v8::Handle<v8::Array> props = v8ctx->Global()->GetPropertyNames();
    for(int i=0;i<props->Length();++i) {
        v8::Local<v8::String> pname = props->Get(i)->ToString();
        v8ctx->Global()->Delete(pname);
    }
}


void CSState::Load()
{
    // \"--file-filter=Graphs (*.graph) | *.graph\"
    char *filename;
    filename=s.e->wnd->GetFilename("Load",false);
    if(!filename) return;

    FILE *fl = fopen(filename,"r");
    if(!fl) return;

    Clear();
    s.e->graphics.ResetView();

    strcpy(last_filename,filename);
    static char titlebuf[1024];
    sprintf(titlebuf,"%s - graphic depictions",last_filename);
    s.e->wnd->SetWindowTitle(titlebuf);

    int version=0;
    int dims;
    char tagline[32];

    fscanf(fl," %s ",tagline);
    if(strcmp(tagline,"version")) {
        sscanf(tagline," %dD ",&dims);
    } else {
        fscanf(fl," %d ",&version);
        fscanf(fl," %dD ",&dims);
    }

    if(dims==3) s.e->use_3d=true;
    else s.e->use_3d=false;

    int id=0,ide=0;

    int nnodes;
    fscanf(fl," %d ",&nnodes);

    for(int i=0;i<nnodes;++i) {
        int sel,nattrs;
        float x,y,z;

        fscanf(fl," %d %f %f %f %d ", &sel, &x, &y, &z, &nattrs);
        CSNode *n = AddNode(x,y,z);
        id2nodes[id++] = n;
        n->selected = sel;
        if(sel) ++nselected;

        for(int j=0;j<nattrs;++j) {
            char buf[128],buf2[128]; char type; int len;
            fscanf(fl," \"%[^\"]\"=%c",buf,&type);
            if(type=='j') {
                fscanf(fl,"%d,",&len);
                char *json = new char[len+1];
                fread(json,1,len,fl); json[len]=0;
                n->a[buf]=CSAttr::FromStringDeferred(json);
                delete [] json;
            } else {
                // old-style data
                fscanf(fl,"%[^\n] ",buf2);
                n->a[buf]=CSAttr::FromStringDeferred(buf2);
            }
        }
    }

    int nedges;
    fscanf(fl," %d ",&nedges);
    for(int i=0;i<nedges;++i) {
        int n1,n2;
        fscanf(fl," %d %d ",&n1,&n2);
        CSEdge *e=AddEdge(id2nodes[n1],id2nodes[n2]);
        id2edges[ide++] = e;

        if(version>=1) {
            /* load edge attributes, too */
            int nattrs;
            fscanf(fl," %d ",&nattrs);
            for(int j=0;j<nattrs;++j) {
                char buf[128],buf2[128]; char type; int len;
                fscanf(fl," \"%[^\"]\"=%c",buf,&type);
                if(type=='j') {
                    fscanf(fl,"%d,",&len);
                    char *json = new char[len+1];
                    fread(json,1,len,fl); json[len]=0;
                    e->a[buf]=CSAttr::FromStringDeferred(json);
                    delete [] json;
                } else {
                    // old-style data
                    fscanf(fl,"%[^\n] ",buf2);
                    e->a[buf]=CSAttr::FromStringDeferred(buf2);
                }
            }
        }
    }

    int nselgroups=0;
    fscanf(fl," %d ",&nselgroups);
    for(int i=0;i<nselgroups;++i) {
        int ninside;
        fscanf(fl," %d ",&ninside);
        for(int j=0;j<ninside;++j) {
            int id;
            fscanf(fl," %d ",&id);
            selgroups[i].insert(id2nodes[id]);
        }
    }

    /* now that we know the id->node map, compile all attributes */
    for(auto &n : nodes) {
        for(auto &&[k,v] : n->a) {
            v.Compile();
        }
    }
    for(auto &e : edges) {
        for(auto &&[k,v] : e->a) {
            v.Compile();
        }
    }

    if(version >= 2) {
        /* load global context */
        int len;
        fscanf(fl," %d ",&len);
        char *buf = new char[len+1];
        fread(buf,1,len,fl);
        buf[len]=0;
        
        v8::Local<v8::Object> obj = s.e->FromJSON(buf)->ToObject();
        s.e->ImportToGlobal(obj);

        delete [] buf;
    }

    int nscripts=0;
    fscanf(fl," %d ",&nscripts);
    for(int i=0;i<nscripts;++i) {
        int len;
        char buftype[16],buftitle[128],bufcode[16384];

        fscanf(fl," %d byte %[^ \n] script %[^\n] ",&len,buftype,buftitle);

        scripts.push_back(CSScript());

        if(!strcmp(buftype,"node")) scripts.back().onNodes=true;
        else scripts.back().onNodes=false;
        scripts.back().name = buftitle;
        fread(bufcode,1,len,fl);
        bufcode[len]=0;
        scripts.back().code = bufcode;    }

    fclose(fl);

    id2nodes.clear();
    id2edges.clear();
}

void CSState::SaveAs()
{
    //\"--file-filter=Graphs (*.graph) | *.graph\"
    char *filename;
    filename=s.e->wnd->GetFilename("Save",true);
    if(!filename || !filename[0]) return;

    strcpy(last_filename,filename);

    static char titlebuf[1024];
    sprintf(titlebuf,"%s - graphic depictions",last_filename);
    s.e->wnd->SetWindowTitle(titlebuf);

    Save();
}

void CSState::Save()
{
    if(!last_filename[0]) SaveAs();

    FILE *fl = fopen(last_filename,"w");
    if(!fl) return;

    fprintf(fl,"version 2\n");

    fprintf(fl,"%s\n",s.e->use_3d?"3D":"2D");

    fprintf(fl,"%d\n",nodes.size());

    int id=0, ide=0;
    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        nodes2id[*i]=id++;
    }

    for(auto i=nodes.begin(); i!=nodes.end(); ++i) {
        fprintf(fl,"%d %.8f %.8f %.8f %d\n",(*i)->selected,(*i)->pos[0],(*i)->pos[1],(*i)->pos[2],(*i)->a.size());
        for(auto j=(*i)->a.begin(); j!=(*i)->a.end(); ++j) {
            fprintf(fl,"\"%s\"=",j->first.c_str());
            fprintf(fl,"%s\n",j->second.ToString().c_str());
        }
    }

    fprintf(fl,"%d\n",edges.size());
    for(auto i=edges.begin(); i!=edges.end(); ++i) {
        edges2id[*i]=ide++;
    }
    for(auto i=edges.begin(); i!=edges.end(); ++i) {
        fprintf(fl,"%d %d ",nodes2id[(*i)->n1], nodes2id[(*i)->n2]);
        fprintf(fl,"%d",(*i)->a.size());
        for(auto j=(*i)->a.begin(); j!=(*i)->a.end(); ++j) {
            fprintf(fl," \"%s\"=",j->first.c_str());
            fprintf(fl,"%s",j->second.ToString().c_str());
        }
        fprintf(fl,"\n");
    }

    fprintf(fl,"%d\n",NSELGROUPS);
    for(int i=0;i<NSELGROUPS;++i) {
        fprintf(fl,"%d",selgroups[i].size());
        for(auto j=selgroups[i].begin();j!=selgroups[i].end();++j) {
            fprintf(fl," %d",nodes2id[*j]);
        }
        fprintf(fl,"\n");
    }

    /* save global vars */
    std::string glob = s.e->ToJSON(v8ctx->Global());
    fprintf(fl,"%d\n",glob.length());
    fwrite(glob.c_str(),glob.length(),1,fl);
    fprintf(fl,"\n");

    fprintf(fl,"%d\n",scripts.size());
    for(int id=0;id<scripts.size();++id) {
        fprintf(fl,"%d byte %s script %s\n", scripts[id].code.length(),scripts[id].onNodes?"node":"graph", scripts[id].name.c_str());
        fwrite(scripts[id].code.c_str(),scripts[id].code.length(),1,fl);
        fprintf(fl,"\n");    
    }

    fclose(fl);

    nodes2id.clear();
    edges2id.clear();
}
