//
// Created by ali on 10/19/23.
//

#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"

// global data tree.
extern struct lyd_node *root_data, *parent_data;


int cmd_yang_leaf_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter value(s) for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }

    // libcli does not support validating mutiple values for same optarg, this is a WA to validate all values.
    for (int i = 0; i < argc; i++)
        if (yang_data_validator(cli, cmd, argv[i], c->cmd_model) != CLI_OK) return CLI_ERROR_ARG;


    struct lysc_node *ne = (struct lysc_node *) c->cmd_model;
    char xpath[256];

    lysc_path(ne, LYSC_PATH_DATA, xpath, 256);

    if (ne != NULL)
        cli_print(cli, "  xpath=%s\r\n", xpath);
    else
        cli_print(cli, "  failed to find yang module\r\n");
    return CLI_OK;

}

int cmd_yang_leaf(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter value for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one value for %s", cmd);
        return CLI_MISSING_ARGUMENT;
    }
    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;

    // get the relative path for leaf to append value to data tree (example ietf-ip:ipv4)
    char relative_xpath[256];
    snprintf(relative_xpath, 256, "%s:%s", y_node->module->name, y_node->name);

    int ret = lyd_new_path2(parent_data, y_node->module->ctx, relative_xpath,
                            argv[0], 0, 0, LYD_NEW_PATH_UPDATE, NULL,
                            NULL);

    if (ret != LY_SUCCESS) {
        cli_print(cli, "Failed to create the yang data node for '%s'\n", y_node->name);
        print_ly_err(ly_err_first(y_node->module->ctx));
        return CLI_ERROR;
    }
    char *result;
    lyd_print_mem(&result, root_data, LYD_XML, 0);
    cli_print(cli, result, NULL);
    return CLI_OK;

}


int register_cmd_leaf_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [leaf-list]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_module = lysc_owner_module(y_node);

    char *cmd_hash = strdup(y_module->name);;
    mode = y_get_curr_mode(y_node);


    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_leaf_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);
    cli_register_optarg(c, "value(s)", CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD | CLI_CMD_OPTION_MULTIPLE,
                        PRIVILEGE_PRIVILEGED, mode,
                        y_node->dsc, NULL, NULL, NULL);

    return 0;
}


int register_cmd_leaf(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [leaf]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_module = lysc_owner_module(y_node);
    char *cmd_hash = strdup(y_module->name);;
    mode = y_get_curr_mode(y_node);


    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_leaf,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    const char *optarg_help;
    LY_DATA_TYPE type = ((struct lysc_node_leaf *) y_node)->type->basetype;
    if (type == LY_TYPE_IDENT)
        optarg_help = creat_help_for_identity_type(y_node);
    else
        optarg_help = y_node->dsc;
//    optarg_help = y_node->dsc;



//    uint type = ((struct lysc_node_leaf *)y_node)->type->basetype;
//    const char * value_type = ly_data_type2str[type];
//    char * value_name = malloc(sizeof("value")+ sizeof(value_type) + 1);
//
//    sprintf(value_name,"value:%s",value_type);
    cli_register_optarg(c, "value", CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD, PRIVILEGE_PRIVILEGED, mode,
                        optarg_help, NULL, yang_data_validator, NULL);

    return 0;
}