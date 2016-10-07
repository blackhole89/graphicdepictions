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

// space.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "space.h"
#define MAX_LOADSTRING 100

// Global Variables:
//HINSTANCE hInst;								// current instance


#ifdef WIN32
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
	srand((unsigned)time(NULL));

	s.wnd->Create();
	s.wnd->InitGL();
	s.wnd->Show();

	s.e->Init(s.wnd);
	s.e->Run();

	s.wnd->Destroy();

	return 0;
}

