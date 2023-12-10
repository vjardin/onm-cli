//
// Created by ali on 11/17/23.
//

#ifndef ONMCLI_DATA_FACTORY_H
#define ONMCLI_DATA_FACTORY_H

#include <libyang/libyang.h>
#include <libyang/parser_data.h>
#include <libyang/tree_data.h>
#include <libyang/printer_data.h>
#include <libyang/tree.h>
#include <libyang/log.h>
#include "lib/libcli/libcli.h"

struct data_tree {
    struct lyd_node *node;
    struct data_tree *prev;
};

struct data_tree *get_config_root_tree();

void free_data_tree_all();

int add_data_node(struct lysc_node *y_node, char *value);

int add_data_node_list(struct lysc_node *y_node, char *argv[], int argc,int index);

int delete_data_node(struct lysc_node *y_node, char *value);

int delete_data_node_list(struct lysc_node *y_node, char *argv[], int argc);

#endif //ONMCLI_DATA_FACTORY_H
