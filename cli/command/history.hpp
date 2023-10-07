#pragma once
#include "alice/alice.hpp"

namespace alice {

class history_command : public command {
public:
    explicit history_command(const environment::ptr &env) :
        command(env, "Perform history AIG operations, the history size is 5") {
        add_flag("--clear, -c", cclear, "clear the history AIG files");
        add_flag("--size, -s", csize, "toggles of show the history size of AIG files");
        add_flag("--add, -a", cadd, "toggles of add the previous optimized AIG file");
        add_option("--replace, -r", index_replace, "toggles of delete the previous optimized AIG file");
    }

    rules validity_rules() const {
        return {};
    }

protected:
    void execute() {    
        if (store<iFPGA::aig_network>().empty()) {
            printf("WARN: there is no any stored AIG file, please refer to the command \"read_aiger\"\n");
            return;
        }

        if (cadd && history_index + 1 <= max_size) {
            iFPGA::aig_network aig = store<iFPGA::aig_network>().current()._storage;
            store<iFPGA::aig_network>()[++history_index] = aig;
        }

        if (history_index + 1 > max_size) {
            printf("WARN: the max history size is %d, please refer to the command \"history -h\"\n", max_size);
            return;
        }

        // using current AIG to replace the indexed history AIG
        if (index_replace == -1) {
            // do nothing
        } else if (index_replace > history_index || index_replace < -1) {
            printf("WARN: the replace index is out of range, please refer to the command \"history -h\"\n");
        } else {
            iFPGA::aig_network aig = store<iFPGA::aig_network>().current()._storage;
            store<iFPGA::aig_network>()[index_replace] = aig; // replace the AIG file
        }

        if (cclear) {
            history_index = -1;
        }

        if (csize) {
            printf("INFO: the history size is %d\n", history_index+1 ); // current aig are the latest version of optimized AIG, and the previous indexed AIGs are the history AIGs.
        }
        
        cclear = false;
        csize = false;
        cadd = false;
        index_replace = -1;
        
        return;
    }

private:
    int max_size = 5;          // the history AIGs are indexed with {0,1,2,3,4}
    int history_index = -1;    
    bool cclear = false;
    bool csize = false;
    bool cadd = false;
    int index_replace = -1;
};
ALICE_ADD_COMMAND(history, "Logic optimization");
}; // namespace alice