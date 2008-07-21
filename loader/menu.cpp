/* Copyright (c) 2007 Mega Man */
#include "menu.h"
#include "graphic.h"
#include "configuration.h"

void Menu::paint(void)
{
	int y, x;
	int z;
	vector<MenuEntry>::iterator i;
	int start;
	int c;

	y = positionY;
	x = positionX;

	if (title != NULL) {
		/** Text colour. */
		static u64 TexCol;
		/** Scale factor for font. */
		static float scale = 1.4f;

		TexCol = GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00);

		gsKit_font_print_scaled(gsGlobal, gsFont, x, y, 3, scale, TexCol,
			title);
		y += 50;
	}

	start = 0;
	if (numberOfMenuEntries > numberOfMenuItems) {
		if ((numberOfMenuEntries - start) < numberOfMenuItems) {
			start = numberOfMenuEntries - numberOfMenuItems;
		} else {
			start = selectedMenu - (numberOfMenuItems / 2);
			if (start < 0) {
				start = 0;
			}
		}
	}
	c = 0;
	z = 2;
	for (i = menuVector.begin(); i != menuVector.end(); i++) {
		if (c >= start) {
			i->paint(x, y, z);
			y += 35;
			z += 2;
		}
		c++;
		if ((c - start) >= numberOfMenuItems) {
			break;
		}
	}
}

int i;

void Menu::addItem(const char *name, executeMenuFn_t *executeFn, void *executeArg, GSTEXTURE *tex)
{
	MenuEntry menuEntry(gsGlobal, gsFont, name, executeFn, executeArg, tex);
	menuVector.push_back(menuEntry);

	selectMenuEntry(selectedMenu);

	numberOfMenuEntries++;
}

int checkItem(void *arg)
{
	MenuEntry *menuEntry = (MenuEntry *) arg;

	menuEntry->switchItem();

	return 0;
}

void Menu::addCheckItem(const char *name, int *value)
{
	MenuEntry menuEntry(gsGlobal, gsFont, name, checkItem, value);
	menuVector.push_back(menuEntry);

	selectMenuEntry(selectedMenu);
	addConfigCheckItem(name, value);

	numberOfMenuEntries++;
}

void Menu::addMultiSelectionItem(const char *name, const char **valueList, int *value, GSTEXTURE *tex)
{
	MenuEntry menuEntry(gsGlobal, gsFont, valueList, checkItem, value, tex);
	menuVector.push_back(menuEntry);

	selectMenuEntry(selectedMenu);
	addConfigCheckItem(name, value);

	numberOfMenuEntries++;
}


void Menu::selectMenuEntry(int selection)
{
	menuVector[selectedMenu].setSelected(false);
	selectedMenu = selection;
	menuVector[selectedMenu].setSelected(true);
}

int Menu::execute(void)
{
	return menuVector[selectedMenu].execute();
}

Menu *Menu::addSubMenu(const char *name)
{
	Menu *subMenu;

	subMenu = new Menu(gsGlobal, gsFont, numberOfMenuItems);
	subMenu->setTitle(name);
	subMenu->setPosition(positionX, positionY);

	addItem(name, setCurrentMenu, subMenu);

	return subMenu;
}

Menu *Menu::getSubMenu(const char *name)
{
	Menu *subMenu;

	subMenu = new Menu(gsGlobal, gsFont, numberOfMenuItems);
	subMenu->setTitle(name);
	subMenu->setPosition(positionX, positionY);

	return subMenu;
}
