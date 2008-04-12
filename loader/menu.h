/* Copyright (c) 2007 Mega Man */
#ifndef _MENU_H_
#define _MENU_H_

#include "stdlib.h"
#include "menuEntry.h"

#include "gsKit.h"
#include "dmaKit.h"
#include "vector"

class Menu {
	protected:
	GSGLOBAL *gsGlobal;

	/** Font used for printing text. */
	GSFONT *gsFont;

	int selectedMenu;
	int numberOfMenuEntries;
	vector<MenuEntry> menuVector;
	int positionX;
	int positionY;
	const char *title;
	/** Maximum number of menu items on one page (display). */
	int numberOfMenuItems;

	public:
	Menu(GSGLOBAL *gsGlobal, GSFONT *gsFont, int numberOfMenuItems) :
		gsGlobal(gsGlobal),
		gsFont(gsFont),
		positionX(0),
		positionY(0),
		title(NULL),
		numberOfMenuItems(numberOfMenuItems)
	{
		selectedMenu = 0;
		numberOfMenuEntries = 0;
	}

	~Menu()
	{
	}

	void paint(void);

	void addItem(const char *name, executeMenuFn_t *executeFn, void *executeArg, GSTEXTURE *tex = NULL);
	void addCheckItem(const char *name, int *value);

	Menu *addSubMenu(const char *name);

	Menu *getSubMenu(const char *name);

	void setPosition(int x, int y)
	{
		positionX = x;
		positionY = y;
	}

	void selectMenuEntry(int selection);

	void setTitle(const char *text)
	{
		title = text;
	}

	const char *getTitle(void)
	{
		return title;
	}

	void selectUp(void)
	{
		int i;

		i = selectedMenu;

		i--;
		if (i >= 0) {
			selectMenuEntry(i);
		}
	}

	void selectDown(void)
	{
		int i;

		i = selectedMenu;

		i++;
		if (i < numberOfMenuEntries) {
			selectMenuEntry(i);
		}
	}

	int execute(void);

	void deleteAll(void){
		numberOfMenuEntries = 0;
		selectedMenu = 0;

		menuVector.clear();
	}

	void deleteEntry(const char *name) {
		vector<MenuEntry>::iterator i;
		for (i = menuVector.begin(); i != menuVector.end(); i++) {
			if (strcmp(i->getName(), name) == 0) {
				menuVector.erase(i);
				numberOfMenuEntries--;
				break;
			}
		}
	}
};

#endif
