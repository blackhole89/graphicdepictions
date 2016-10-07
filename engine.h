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

#ifndef _ENGINE_H_
#define _ENGINE_H_

#ifndef WIN32
#include <sys/time.h>
#else
#include <sys/timeb.h>
#include <sys/types.h>
#endif
#include "stdafx.h"

struct CSScript {
    std::string name;
    std::string code;
    bool onNodes;
};

class CSState {
public:
    struct CSAttr {
        enum {
            TY_FLOAT,
            TY_FLOAT3,
            TY_INT,
            TY_BITS
        } type;
        union {
            float d_float;
            float d_float3[3];
            int d_int;
            int d_bits;
        } data;

        CSAttr(float);
        CSAttr(float,float,float);
        CSAttr(int,bool bits=false);
        CSAttr();
    };
    typedef std::map<std::string, CSAttr> attrs;

    class CSNode;
    class CSEdge;

    struct CSEdge {
        attrs a;

        CSNode *n1, *n2;

        bool selected;
    };

    struct CSNode {
        attrs a;

        float pos[3];

        std::set<CSNode*> adj;
        std::set<CSEdge*> adje;

        bool selected;    };

    std::set<CSNode*> nodes;
    std::set<CSEdge*> edges;

    int nselected;

    std::vector<CSScript > scripts;

    #define NSELGROUPS 10
    std::set<CSNode*> selgroups[NSELGROUPS];

    //std::map<std::string, CSNode*> nodesbyname;

    /* basic stuff */
    CSNode* AddNode(float x, float y, float z);
    CSEdge* AddEdge(CSNode *n1, CSNode *n2);

    void DelNode(CSNode *n);
    void DelEdge(CSNode *n1, CSNode *n2);
    void DelEdge(CSEdge *e);

    /* medium stuff */
    void GrowSelection();
    void ShrinkSelection();
    void SelectConnected();
    void InvertSelection();

    void SaveSelectionGroup(int id);
    void LoadSelectionGroup(int id);

    void DuplicateSelection();
    void ExtrudeSelection();

    /* advanced stuff */
    void MkDiscreteCube(int dim);
    void MkGnp(int n, float p);
    void OrderFromSelection();
    bool RelaxationStep(float tlength);
    void RoundPositions(float gridsize);

    /* load/save */
    char last_filename[1024];
    void SaveAs();
    void Save();
    void Load();
    void Clear();};

class CSEngine {
	friend class CSGraphics;
	friend class CSUI;
public:
	CSMainWindow *wnd;
	CSGraphics graphics;

public:
    CSState st;

    bool use_3d;

    enum {
        AC_NOTHING,
        AC_GO,
        AC_SCALE,
        AC_SELECT,
        AC_RELAX
    } action;

    v8::HandleScope v8hs;
    v8::Persistent<v8::Context> v8ctx;
    v8::Persistent<v8::ObjectTemplate> node_templ, set_templ;

    std::vector<CSScript > scripts;
    char err_buf[4096];

    void LoadScripts();
    void SaveScript(int id);

    std::deque<std::string> recent;
public:
	pthread_mutex_t loading_mutex;
	pthread_cond_t loading_cond;

	bool keys[256];

	long f1,f2,sf1,sf2,df1,df2,tdelta;

	/* script editor state */
	int editor_index;
    bool editor_local;
    char editor_tbuf[64];
    char editor_buf[4096];
    bool editor_is_pernode;

	void ScriptListEntry(CSScript *s, int id, bool local);

	void Init(CSMainWindow *wndw);
	void Run();
	void RunLogic();

	void Click(int btn, int x,int y);

    void StartDragging(int x0, int y0);
    void CancelDragging();
	void SetSelectionRect(int x0, int y0, int x1, int y1);

	bool CompileScript(const char *code,const char *name, v8::Handle<v8::Script> &out);
	bool RunScriptForNode(CSState::CSNode *n, v8::Handle<v8::Script> script);
	bool RunScript(v8::Handle<v8::Script> script);
};

typedef struct {
	long x,y,z;
} SPosition;

#endif
