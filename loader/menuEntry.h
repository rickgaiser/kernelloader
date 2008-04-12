/* Copyright (c) 2007 Mega Man */
#ifndef _MENU_ENTRY_H_
#define _MENU_ENTRY_H_

#include "gsKit.h"
#include "dmaKit.h"
#include "vector"

typedef int (executeMenuFn_t)(void *arg);

class MenuEntry {
	protected:
	GSGLOBAL *gsGlobal;
	GSFONT *gsFont;
	const char *name;
	bool selected;
	executeMenuFn_t *executeFn;
	void *executeArg;
	bool checkItem;
	int *checkValue;

	public:
	MenuEntry(GSGLOBAL *gsGlobal, GSFONT *gsFont, const char *name, executeMenuFn_t *executeFn, void *executeArg) :
		gsGlobal(gsGlobal), gsFont(gsFont),
		name(name), selected(false),
		executeFn(executeFn), executeArg(executeArg),
		checkItem(false), checkValue(NULL)
	{
	}

	MenuEntry(GSGLOBAL *gsGlobal, GSFONT *gsFont, const char *name, executeMenuFn_t *executeFn, int *checkValue) :
		gsGlobal(gsGlobal), gsFont(gsFont),
		name(name), selected(false),
		executeFn(executeFn), executeArg(this),
		checkItem(true), checkValue(checkValue)
	{
	}

	MenuEntry(const MenuEntry &other) :
		gsGlobal(other.gsGlobal),
		gsFont(other.gsFont),
		name(other.name),
		selected(other.selected),
		executeFn(other.executeFn),
		checkItem(other.checkItem),
		checkValue(other.checkValue)
	{
		if (checkItem) {
			executeArg = this;
		} else {
			executeArg = other.executeArg;
		}
	}

	~MenuEntry()
	{
	}

	const char *getName(void)
	{
		return name;
	}

	bool isSelected(void)
	{
		return selected;
	}

	void setSelected(bool state)
	{
		selected = state;
	}

	void paint(int x, int y);

	int execute(void);

	bool switchCheckItem()
	{
		if (checkItem) {
			*checkValue = !(*checkValue);
			return *checkValue;
		} else {
			return false;
		}
	}
};

#endif
