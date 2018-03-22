//
//  model.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "model.hpp"

namespace Table {

Model::Model() {
    version_count = 0;
}

int get_header_index(Model* model, std::string header) {
    for (int j = 0; j < model->headers.size(); ++j) {
        if (model->headers[j] == header) {
            return j;
        }
    }

    return -1;
}

void set_key(Model* model, std::vector<std::string> key) {
    if (model->headers.empty()) {
        throw "Headers not set yet";
    }

    if (!model->key.empty()) {
        throw "Key already set";
    }

    for (auto& header : key) {
        auto index = get_header_index(model, header);
        if (index == -1) {
            throw "Header used as key is not present in table";
        }
        model->key.push_back(index);
    }
}

void insert_row(Model* model, std::vector<std::string> row) {
    if (row.size() != model->headers.size()) {
        throw "Row and header has different number of columns";
    }

    model->version_count++;

    if (!model->key.empty()) {
        int index = -1;
        for (int i = 0; i < model->rows.size(); ++i) {
            bool match = true;
            for (int j = 0; j < model->key.size(); ++j) {
                if (model->rows[i][model->key[j]] != row[model->key[j]]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                index = i;
                break;
            }
        }

        if (index != -1) {
            model->rows[index] = std::move(row);
            return;
        }
    }

    model->rows.push_back(std::move(row));
}

}
