//
// Created by ali on 10/9/23.
//

/*
 * cli commands generator functions
 * */
#include "yang_cmd_generator.h"
#include "yang_cmd.h"
#include "../utils.h"


/**
 * register command in cli for a container node
 * @param name
 * @param parent_name
 * @param cli
 * @return
 */
int register_cmd_container(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure setting for %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_container, PRIVILEGE_UNPRIVILEGED, mode,
                         help);
}

/**
 * register command in cli for a leaf node
 * @param name
 * @param parent_name
 * @param cli
 * @return
 */
int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_leaf,
                                                 PRIVILEGE_PRIVILEGED, mode, help);
    cli_register_optarg(c, "value", CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, mode,
                        y_node->dsc, NULL, NULL, NULL);

    return 0;
}

int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s", y_node->name);
    unsigned int mode;
    if (y_node->parent == NULL)
        mode = MODE_CONFIG;
    else
        mode = str2int_hash((char *) y_node->parent->name);
    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    LYSC_TREE_DFS_BEGIN(child_list, child) {
            if (child->flags & LYS_KEY) {
                cli_register_optarg(c, child->name, CLI_CMD_ARGUMENT, PRIVILEGE_PRIVILEGED, mode,
                                    child->dsc, NULL, NULL, NULL);
                break;
            }
        LYSC_TREE_DFS_END(child_list->next, child);
    }


    return 0;
}

static void register_node_routine(struct cli_def *cli, struct lysc_node *schema) {
    if (schema->flags & LYS_CONFIG_R) {
        return;
    }
    switch (schema->nodetype) {
        case LYS_CONTAINER:
            printf("TRACE: register CLI command for container: %s\r\n", schema->name);
            register_cmd_container(cli, schema);
            break;
        case LYS_LEAF:
            printf("TRACE: register CLI command for leaf: %s\r\n", schema->name);
            register_cmd_leaf(cli, schema);
            break;
        case LYS_LIST:
            printf("TRACE: register CLI command for list: %s\r\n", schema->name);
            register_cmd_list(cli, schema);
            break;
            // Add cases for other node types as needed
        default:
            return;
    }
    return;
}

/**
 * register  commands from a yang schema.
 * @param schema  lysc_node schema the root node
 * @param cli     libcli cli
 * @return
 */
int register_commands_schema(struct lysc_node *schema, struct cli_def *cli) {
    printf("DEBUG:commands.c: registering schema for  `%s`\n", schema->name);
    struct lysc_node *child = NULL;
    LYSC_TREE_DFS_BEGIN(schema, child) {
            register_node_routine(cli, child);
        LYSC_TREE_DFS_END(schema->next, child);
    }
    printf("DEBUG:commands.c: schema `%s` registered successfully\r\n", schema->name);

}

int unregister_commands_schema(struct lysc_node *schema, struct cli_def *cli) {
    printf("DEBUG:commands.c: registering schema for  `%s`\n", schema->name);
    struct lysc_node *child = NULL;
    LYSC_TREE_DFS_BEGIN(schema, child) {
            if (schema != NULL)
                cli_unregister_command(cli,schema->name);
        LYSC_TREE_DFS_END(schema->next, child);
    }
    printf("DEBUG:commands.c: schema `%s` registered successfully\r\n", schema->name);

}

int cmd_yang2cmd_remove(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  specify yang module name to generate commands from");
        return CLI_OK;
    }

    char *module_name = (char *) argv[0];
    const struct lys_module *module = get_module_schema(module_name);
    if (module == NULL) {
        cli_print(cli, "  ERROR: module `%s` not found, \n"
                       "  please make sure to set the correct search dir for yang,\n"
                       "  use command 'yang search_dir set /path/to/yang_modules' in enable mode", module_name);
        return CLI_ERROR;
    } else if (module->compiled == NULL) {
        cli_print(cli, "  ERROR: module `%s` was not complied check onm_cli logs for details\n", module_name);
        return CLI_ERROR;
    } else if (module->compiled->data == NULL) {
        if (module->parsed == NULL) {
            cli_print(cli, "  ERROR: module `%s` is loaded but not parsed.. check logs for details\n", module_name);
            return CLI_ERROR;
        } else if (module->parsed->augments != NULL) {
            cli_print(cli, "  WARN: module `%s` is loaded but not parsed as it's augmenting `%s`\n"
                           "  please run yang generate for the augmented module", module_name,
                      module->parsed->augments->node.name);
            return CLI_ERROR;
        }
    }

    unregister_commands_schema(module->compiled->data, cli);
}

int cmd_yang2cmd_generate(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    printf("DEBUG:commands.c:cmd_yang2cmd_generate(): executing command `%s`\n", cmd);

    if (argc == 0) {
        cli_print(cli, "  please specify yang module name");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  specify yang module name to generate commands from");
        return CLI_OK;
    }
    char *module_name = (char *) argv[0];
    const struct lys_module *module = get_module_schema(module_name);
    if (module == NULL) {
        cli_print(cli, "  ERROR: module `%s` not found, \n"
                       "  please make sure to set the correct search dir for yang,\n"
                       "  use command 'yang search_dir set /path/to/yang_modules' in enable mode", module_name);
        return CLI_ERROR;
    } else if (module->compiled == NULL) {
        cli_print(cli, "  ERROR: module `%s` was not complied check onm_cli logs for details\n", module_name);
        return CLI_ERROR;
    } else if (module->compiled->data == NULL) {
        if (module->parsed == NULL) {
            cli_print(cli, "  ERROR: module `%s` is loaded but not parsed.. check logs for details\n", module_name);
            return CLI_ERROR;
        } else if (module->parsed->augments != NULL) {
            struct lysp_import *imported_modules = module->parsed->imports;

            // load impoted recursivly
            LY_ARRAY_COUNT_TYPE i;
            LY_ARRAY_FOR(imported_modules,i){
                cli_print(cli, "  INFO: module `%s` importing`%s` module\n"
                               "  loading `%s` as well", module_name,
                          imported_modules[i].name,imported_modules[i].name);
                const char *new_argv = {imported_modules[i].name};
                cmd_yang2cmd_generate(cli, c, cmd, (char**)&new_argv, argc);
            }
            return CLI_OK;

        }
        return CLI_OK;
    }

    // if the module is already registered remove it first
    unregister_commands_schema(module->compiled->data,cli);

    register_commands_schema(module->compiled->data, cli);
    cli_print(cli, "  yang commands generated successfully");
    return CLI_OK;

}


int cmd_yang_searchdir_set(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  specify yang dir");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  yang dir was not specified");
        return CLI_MISSING_ARGUMENT;
    }

    set_yang_searchdir(argv[0]);
    return CLI_OK;

}

int cmd_yang_searchdir_unset(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 0) {
        cli_print(cli, "  specify yang dir");
        return CLI_INCOMPLETE_COMMAND;
    }
    if (strcmp(argv[0], "?") == 0) {
        cli_print(cli, "  yang dir was not specified");
        return CLI_MISSING_ARGUMENT;
    }

    unset_yang_searchdir(argv[0]);
    return CLI_OK;

}

int cmd_yang_list_searchdirs(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {
    if (argc == 1) {
        if (strcmp(argv[0], "?") == 0) {
            cli_print(cli, "  <cr>");
            return CLI_MISSING_ARGUMENT;
        }
    }

    const char *const *dir_list = get_yang_searchdirs();
    while (*dir_list != NULL) {
        cli_print(cli, "  [+] %s", *dir_list);
        ++dir_list;
    }


}

int yang_cmd_generator_init(struct cli_def *cli) {
    struct cli_command *yang_cmd = cli_register_command(cli, NULL, NULL,
                                                        "yang", NULL, PRIVILEGE_PRIVILEGED,
                                                        MODE_EXEC, "yang settings");
    if (yang_cmd == NULL)
        printf("failed\n");
    struct cli_command *yang_set_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                            "set", NULL, PRIVILEGE_UNPRIVILEGED,
                                                            MODE_EXEC, "set yang settings");
    struct cli_command *yang_unset_cmd = cli_register_command(cli, yang_cmd, NULL,
                                                              "unset", NULL, PRIVILEGE_UNPRIVILEGED,
                                                              MODE_EXEC, "unset yang settings");
    cli_register_command(cli, yang_set_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_set, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "set yang search dir: yang set searchdir <path/to/modules>");
    cli_register_command(cli, yang_unset_cmd, NULL,
                         "searchdir", cmd_yang_searchdir_unset, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "unset yang search dir: yang set searchdir <path/to/modules>");

    cli_register_command(cli, yang_cmd, NULL,
                         "list-seachdir", cmd_yang_list_searchdirs, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "list yang serachdirs");

    cli_register_command(cli, yang_cmd, NULL,
                         "load-module", cmd_yang2cmd_generate, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "load yang module to onm_cli:yang load-module <module-name>");

    cli_register_command(cli, yang_cmd, NULL,
                         "remove-module", cmd_yang2cmd_remove, PRIVILEGE_PRIVILEGED,
                         MODE_EXEC, "remove yang module from onm_cli:yang remove-module <module-name>");

    return 0;
}
