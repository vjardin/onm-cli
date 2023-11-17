//
// Created by ali on 10/19/23.
//

#include "y_utils.h"
#include "yang_core.h"
#include "data_validators.h"
#include "data_factory.h"

// global data tree.
extern struct lyd_node *root_data, *parent_data;

int cmd_yang_list(struct cli_def *cli, struct cli_command *c, const char *cmd, char *argv[], int argc) {

    if (argc == 0) {
        cli_print(cli, "ERROR: please enter %s of %s entry", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    } else if (argc > 1) {
        cli_print(cli, "ERROR: please enter one Key(%s) for the list entry of %s", c->optargs->name, cmd);
        return CLI_MISSING_ARGUMENT;
    }

    struct lysc_node *y_node = (struct lysc_node *) c->cmd_model;


    int ret = add_data_node(y_node, c, argv[0]);


    if (ret != LY_SUCCESS) {
        fprintf(stderr, "Failed to create the data tree\n");
        print_ly_err(ly_err_first(y_node->module->ctx));
        cli_print(cli, "failed to execute command, error with adding the data node.");
        return CLI_ERROR;
    }

    char *mod_str = malloc(strlen(cmd) + strlen(argv[0]) + 3);
    sprintf(mod_str, "%s[%s]", (char *) cmd, argv[0]);

    int mode = y_get_next_mode(y_node);

    cli_push_configmode(cli, mode, mod_str);
    return CLI_OK;
}


int register_cmd_list(struct cli_def *cli, struct lysc_node *y_node) {
    char help[100];
    sprintf(help, "configure %s (%s) [list]", y_node->name, y_node->module->name);
    unsigned int mode;
    const struct lys_module *y_root_module = lysc_owner_module(y_node);

    char *cmd_hash = strdup(y_root_module->name);;
    mode = y_get_curr_mode(y_node);

    struct cli_command *c = cli_register_command(cli, NULL, y_node, y_node->name, cmd_yang_list,
                                                 PRIVILEGE_PRIVILEGED, mode, cmd_hash, help);

    const struct lysc_node *child_list = lysc_node_child(y_node);
    const struct lysc_node *child;
    LY_LIST_FOR(child_list, child) {
        if (child->flags & LYS_KEY) {
            const char *optarg_help;
            LY_DATA_TYPE type = ((struct lysc_node_leaf *) child)->type->basetype;
            if (type == LY_TYPE_IDENT)
                optarg_help = creat_help_for_identity_type((struct lysc_node *) child);
            else
                optarg_help = child->dsc;
            cli_register_optarg(c, child->name, CLI_CMD_ARGUMENT | CLI_CMD_DO_NOT_RECORD, PRIVILEGE_PRIVILEGED,
                                mode, optarg_help, NULL, yang_data_validator, NULL);
        }
    }
    return 0;
}
