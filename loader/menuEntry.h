/* Copyright (c) 2007 Mega Man */
#ifndef _MENU_ENTRY_H_
#define _MENU_ENTRY_H_

#include "gsKit.h"
#include "dmaKit.h"
#include "vector"
#include "gsKit.h"

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
	bool multiItem;
	const char **valueList;
	int *multiValue;
	GSTEXTURE *tex;

	public:
	/** Normal menu entry with slectable icon. */
	MenuEntry(GSGLOBAL *gsGlobal, GSFONT *gsFont, const char *name, executeMenuFn_t *executeFn, void *executeArg, GSTEXTURE *tex) :
		gsGlobal(gsGlobal), gsFont(gsFont),
		name(name), selected(false),
		executeFn(executeFn), executeArg(executeArg),
		checkItem(false), checkValue(NULL),
		multiItem(false), valueList(NULL), multiValue(NULL),
		tex(tex)
	{
	}

	/** Menu entry for check items. */
	MenuEntry(GSGLOBAL *gsGlobal, GSFONT *gsFont, const char *name, executeMenuFn_t *executeFn, int *checkValue) :
		gsGlobal(gsGlobal), gsFont(gsFont),
		name(name), selected(false),
		executeFn(executeFn), executeArg(this),
		checkItem(true), checkValue(checkValue),
		multiItem(false), valueList(NULL), multiValue(NULL),
		tex(NULL)
	{
	}

	/** Menu entry for multi selection items. */
	MenuEntry(GSGLOBAL *gsGlobal, GSFONT *gsFont, const char **valueList, executeMenuFn_t *executeFn, int *multiValue, GSTEXTURE *tex) :
		gsGlobal(gsGlobal), gsFont(gsFont),
		name(valueList[*multiValue]), selected(false),
		executeFn(executeFn), executeArg(this),
		checkItem(false), checkValue(NULL),
		multiItem(true), valueList(valueList), multiValue(multiValue),
		tex(tex)
	{
	}

	MenuEntry(const MenuEntry &other) :
		gsGlobal(other.gsGlobal),
		gsFont(other.gsFont),
		name(other.name),
		selected(other.selected),
		executeFn(other.executeFn),
		checkItem(other.checkItem),
		checkValue(other.checkValue),
		multiItem(other.multiItem),
		valueList(other.valueList),
		multiValue(other.multiValue),
		tex(other.tex)
	{
		if (checkItem || multiItem) {
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

	void paint(int x, int y, int z);

	int execute(void);

	bool switchItem()
	{
		if (checkItem) {
			*checkValue = !(*checkValue);
			return *checkValue;
		} else {
			if (multiItem) {
				(*multiValue)++;
				if (valueList[*multiValue] == NULL) {
					*multiValue = 0;
				}
				name = valueList[*multiValue];
			} else {
				return false;
			}
		}
	}
};

#endif
