#include <fstream>
#include <iostream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include "preferences.hpp"

using std::cout;
using std::endl;
using std::runtime_error;
using std::string;
using std::ostringstream;
using std::vector;

static const int BUFFER_SIZE = 256;
static const xmlChar *root_name = xmlCharStrdup("nxsummary");
static const xmlChar *item_name = xmlCharStrdup("item");
static const xmlChar *path_name = xmlCharStrdup("path");
static const xmlChar *label_name = xmlCharStrdup("label");
static const xmlChar *operation_name = xmlCharStrdup("operation");

namespace nxsum {
  static bool canRead(const string &filename) {
    if (filename.size() <= 0)
    {
      return false;
    }
    std::ifstream temp(filename.c_str(), std::ifstream::in);
    temp.close();
    if (temp.fail())
      {
        temp.clear(std::ifstream::failbit);
        return false;
      }
    else
      {
        return true;
      }
  }

  void addItem(vector<Item> &preferences, const string &path,
               const string &label) {
    Item item;
    item.path = path;
    item.label = label;
    preferences.push_back(item);
  }

  void addItem(vector<Item> &preferences, const string &path,
               const string &label, const string &operation) {
    Item item;
    item.path = path;
    item.label = label;
    item.operation = operation;
    preferences.push_back(item);
  }

  void setDefaultPreferences(vector<Item> &preferences) {
    addItem(preferences, "/entry/title", "TITLE");
    addItem(preferences, "/entry/notes", "NOTES");
    addItem(preferences, "/entry/start_time", "START TIME");
    addItem(preferences, "/entry/end_time", "END TIME");
    addItem(preferences, "/entry/duration", "DURATION");
    addItem(preferences, "/entry/proton_charge", "PROTON CHARGE");
    addItem(preferences, "/entry/sample/name", "SAMPLE NAME");
    addItem(preferences, "/entry/sample/nature", "SAMPLE NATURE");
    addItem(preferences, "/entry/sample/type", "SAMPLE TYPE");
    addItem(preferences, "/entry/sample/identifier", "SAMPLE IDENTIFIER");
  /*
static const char*  TOTAL_COUNTS_PATH = "";
static const string TOTAL_COUNTS_LABEL = "TOTAL COUNTS";
static const char*  TOTAL_MON_COUNTS_PATH = "";
static const string TOTAL_MON_COUNTS_LABEL = "MONITOR COUNTS";
static const char*  PI_PATH = "";
static const string PI_LABEL = "PRICINPLE INVESTIGATOR";
  */
  }

#if defined(LIBXML_TREE_ENABLED)
  void loadPreferences(const string &filename, xmlDocPtr &doc) {
    xmlLineNumbersDefault(1);
    doc = xmlReadFile(filename.c_str(), NULL, 0);
    if (doc == NULL)
      {
        ostringstream s;
        s << "Could not read configuration file \"" << filename << "\"";
        throw runtime_error(s.str());
      }
  }

  void cleanupXml(xmlDocPtr &doc) {
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }

  void loadPreference(xmlNodePtr &item_node, vector<Item> &preferences) {
    if (item_node == NULL)
      {
        return;
      }
    if (item_node->type != XML_ELEMENT_NODE)
      {
        return;
      }
    if (! xmlStrEqual(item_node->name, item_name))
      {
        return;
      }
    if (item_node->properties == NULL)
      {
        return;
      }
    Item item;
    if (xmlHasProp(item_node, path_name))
      {
        item.path = (char *) xmlGetProp(item_node, path_name);
      }
    else
      {
        ostringstream s;
        s << "Could not find path attribute in configuration file (line ";
        s << xmlGetLineNo(item_node) << ")";
        throw runtime_error(s.str());
      }
    if (xmlHasProp(item_node, label_name))
      {
        item.label = (char *) xmlGetProp(item_node, label_name);
      }
    else
      {
        ostringstream s;
        s << "Could not find label attribute in configuration file (line ";
        s << xmlGetLineNo(item_node) << ")";
        throw runtime_error(s.str());
      }
    if (xmlHasProp(item_node, operation_name))
      {
        item.operation = (char *)xmlGetProp(item_node, operation_name);
      }

    preferences.push_back(item);
  }

  void loadPreferences(xmlNodePtr &root_node, vector<Item> &preferences) {
    xmlNodePtr cur_node = NULL;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {
      if ((cur_node->type == XML_ELEMENT_NODE)
                                && xmlStrEqual(cur_node->name, root_name)) {
        for (cur_node = cur_node->children; cur_node; cur_node = cur_node->next) {
          loadPreference(cur_node, preferences);
        }
        return;
      }
    }
  }

  void privateLoadPreferences(const string &filename,
                              vector<Item> &preferences) {
    xmlDocPtr doc = NULL; // document pointer
    loadPreferences(filename, doc);
    xmlNodePtr root_node = xmlDocGetRootElement(doc);
    loadPreferences(root_node, preferences);
    cleanupXml(doc);
  }

  void loadPreferences(const string &filename, vector<Item> &preferences) {
    // work with supplied configuration
    if (filename.size() > 0)
      {
        if (canRead(filename))
          {
            privateLoadPreferences(filename, preferences);
            return;
          }
        else
          {
            cout << "Cannot read configuration file \"" << filename << "\""
                 << endl;
          }
      }

    // user's configuration
    string user_config = string(getenv("HOME")) + string("/.nxsummary.conf");
    if (canRead(user_config))
      {
        privateLoadPreferences(user_config, preferences);
        return;
      }

    // system wide configuration
    string sys_config("/etc/nxsummary.conf");
    if (canRead(sys_config))
      {
        privateLoadPreferences(sys_config, preferences);
        return;
      }

    // default configuration compiled in
    setDefaultPreferences(preferences);
  }
#else
  void loadPreferences(std::string &filename, std::vector<Item> &preferences) {
    cout << "LIBXML2 tree support not present. Using default preferences."
         << endl;
    setDefaultPreferences(preferences);
  }
#endif    

  void writePreference(xmlNodePtr &parent, const Item &preference) {
    xmlNodePtr node = xmlNewChild(parent, NULL, item_name, NULL);
    xmlNewProp(node, path_name, BAD_CAST preference.path.c_str());
    xmlNewProp(node, label_name, BAD_CAST preference.label.c_str());
  }

  void writePreferences(const string &filename,
                        const vector<Item> &preferences) {
#if defined(LIBXML_TREE_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
    // set up variables for creating the document
    xmlDocPtr doc = NULL;        // document pointer
    xmlNodePtr root_node = NULL; // node pointers
    xmlNodePtr node = NULL;      // node pointers
    char buff[256];
    int i, j;

    // create the document and the root node
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, root_name);
    xmlDocSetRootElement(doc, root_node);

    for (vector<Item>::const_iterator it = preferences.begin() ; it != preferences.end() ; it++ ) {
      writePreference(root_node, *it);
    }

    // write out to file
    xmlSaveFormatFileEnc(filename.c_str(), doc, "UTF-8", 1);
    cleanupXml(doc);
#else
    throw runtime_error(
                      "LIBXML2 tree support not present. Cannot write config");
#endif
  }
}
