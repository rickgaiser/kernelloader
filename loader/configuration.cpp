/* Copyright (c) 2007 Mega Man */
#include <vector>

#include "configuration.h"
#include "graphic.h"
#include "fileXio_rpc.h"

#define MAXIMUM_CONFIG_LENGTH 2048
/** FIle where configuration is stored. */
#define CONFIG_FILE CONFIG_DIR "/config.txt"
/** Directory where configuration is stored. */
#define CONFIG_DIR "mc0:kloader"

typedef enum {
	CLASS_CONFIG_CHECK,
	CLASS_CONFIG_TEXT
} configuration_class_type_t;

class ConfigurationItem {
	protected:
	const char *name;
	configuration_class_type_t type;

	public:
	  ConfigurationItem(const char *name,
		configuration_class_type_t type):name(name), type(type) {
	} configuration_class_type_t getType(void) {
		return type;
	}

	const char *getName(void) {
		return name;
	}
};

class ConfigurationCheckItem:ConfigurationItem {
	protected:
	int *value;

	public:
	  ConfigurationCheckItem(const char *name,
		int *value):ConfigurationItem(name, CLASS_CONFIG_CHECK), value(value) {
	} int writeData(FILE * fd) {
		static char text[MAXIMUM_CONFIG_LENGTH];
		int len;

		//printf("write: %s=%d 0x%x\n", name, *value, value);
		len = snprintf(text, MAXIMUM_CONFIG_LENGTH, "%s=%d\n", name, *value);
		return fwrite(text, 1, len, fd);
	}

	void readData(const char *buffer) {
		//printf("read: %s=%s 0x%x\n", name, buffer, value);
		*value = atoi(buffer);
	}
};

class ConfigurationTextItem:ConfigurationItem {
	protected:
	char *value;
	int maxlen;

	public:
	  ConfigurationTextItem(const char *name, char *value,
		int maxlen):ConfigurationItem(name, CLASS_CONFIG_TEXT), value(value),
		maxlen(maxlen) {
	} int writeData(FILE * fd) {
		return fprintf(fd, "%s=%s\n", name, value);
	}

	void readData(const char *buffer) {
		strncpy(value, buffer, maxlen);
		value[maxlen - 1] = 0;
	}
};

vector < ConfigurationItem * >configurationVector;

extern "C" {

	int loadConfiguration(void) {
		static char buffer[MAXIMUM_CONFIG_LENGTH];
		FILE *fd;

		fd = fopen(CONFIG_FILE, "rt");
		if (fd != NULL) {
			while (!feof(fd)) {
				char *val;

				if (fgets(buffer, MAXIMUM_CONFIG_LENGTH, fd) == NULL) {
					break;
				}
				val = strchr(buffer, '=');

				/* remove carriage return. */
				val[strlen(val) - 1] = 0;
				if (val != NULL) {
					vector < ConfigurationItem * >::iterator i;

					*val = 0;
					val++;
					for (i = configurationVector.begin();
						i != configurationVector.end(); i++) {
						const char *name = (*i)->getName();

						if (strcmp(buffer, name) == 0) {
							switch ((*i)->getType()) {
							case CLASS_CONFIG_CHECK:
								{
									ConfigurationCheckItem *item;

									item = (ConfigurationCheckItem *) * i;
									item->readData(val);
								}
								break;
							case CLASS_CONFIG_TEXT:
								{
									ConfigurationTextItem *item;

									item = (ConfigurationTextItem *) * i;
									item->readData(val);
								}
								break;
							default:
								error_printf("Unsupported configuration type.");
								break;
							}
						}
					}
				}
			}
			fclose(fd);
			return 0;
		} else {
			return -1;
		}
	}

	void saveConfiguration(void) {
		FILE *fd;

		fd = fopen(CONFIG_FILE, "wt");
		if (fd == NULL) {
			fileXioMkdir(CONFIG_DIR, 0777);
			fd = fopen(CONFIG_FILE, "wt");
		}
		if (fd != NULL) {
			vector < ConfigurationItem * >::iterator i;

			for (i = configurationVector.begin();
				i != configurationVector.end(); i++) {
				switch ((*i)->getType()) {
				case CLASS_CONFIG_CHECK:
					{
						ConfigurationCheckItem *item;

						item = (ConfigurationCheckItem *) * i;
						item->writeData(fd);
					}
					break;
				case CLASS_CONFIG_TEXT:
					{
						ConfigurationTextItem *item;

						item = (ConfigurationTextItem *) * i;
						item->writeData(fd);
					}
					break;
				default:
					error_printf("Unsupported configuration type.");
					break;
				}
			}
			fclose(fd);
		} else {
			error_printf("Failed to save configuration on MC0. Please insert memory card.");
		}
	}

}

static void addConfigurationItem(ConfigurationItem * item)
{
	configurationVector.push_back(item);
}

void addConfigCheckItem(const char *name, int *value)
{
	ConfigurationCheckItem *item;

	item = new ConfigurationCheckItem(name, value);
	//printf("name %s value 0x%x\n", name, value);

	addConfigurationItem((ConfigurationItem *) item);
}

void addConfigTextItem(const char *name, char *value, int maxlen)
{
	ConfigurationTextItem *item;

	item = new ConfigurationTextItem(name, value, maxlen);

	addConfigurationItem((ConfigurationItem *) item);
}
