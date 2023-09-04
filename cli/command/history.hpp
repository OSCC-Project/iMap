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

        if (store<iFPGA::aig_network>().size() > max_size) {
            printf("WARN: the history size is %d, please refer to the command \"history -h\"\n", max_size);
            return;
        }

        if (cclear) {
            iFPGA::aig_network aig = store<iFPGA::aig_network>().current()._storage;
            store<iFPGA::aig_network>().clear();
            store<iFPGA::aig_network>().extend();
            store<iFPGA::aig_network>().current() = aig;
        }

        if (csize) {
            printf("INFO: the history size is %d\n", (int)store<iFPGA::aig_network>().size() - 1);
        }

        if (cadd) {
            iFPGA::aig_network aig = store<iFPGA::aig_network>().current()._storage;
            store<iFPGA::aig_network>().extend(); // extend the current AIG vector
            store<iFPGA::aig_network>().current() = aig;
        }

        if (index_replace > (int)store<iFPGA::aig_network>().size()) {
            printf("WARN: the replace index is out of range, please refer to the command \"history -h\"\n");
            return;
        } else {
            iFPGA::aig_network aig = store<iFPGA::aig_network>().current()._storage;
            store<iFPGA::aig_network>()[index_replace - 1] = aig; // replace the AIG file
        }
        return;
    }

private:
    uint32_t max_size = 5;
    bool cclear = false;
    bool csize = false;
    bool cadd = false;
    int index_replace = 0;
};
ALICE_ADD_COMMAND(history, "Logic optimization");
}; // namespace alice