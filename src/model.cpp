//
//  model.cpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#include "model.hpp"

namespace Table {

BasicModel::BasicModel() {
    version_count = 0;
}

BasicModel::BasicModel(std::vector<std::string> headers,
                       std::vector<std::string> key) {
    version_count = 1;
    this->headers = std::move(headers);

    if (this->headers.empty()) {
        throw "Headers not set yet";
    }

    for (auto& header : key) {
        auto index = get_header_index(this, header);
        if (index == -1) {
            throw "Header used as key is not present in table";
        }
        this->key_.push_back(index);
    }
}

void BasicModel::insert_row(std::vector<std::string> row) {
    if (row.size() != headers.size()) {
        throw "Row and header has different number of columns";
    }

    version_count++;

    if (!key_.empty()) {
        int index = -1;
        for (int i = 0; i < data.size(); ++i) {
            bool match = true;
            for (auto j : key_) {
                if (data[i][j] != row[j]) {
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
            data[index] = std::move(row);
            return;
        }
    }

    data.push_back(std::move(row));
}

int get_header_index(Model* model, std::string header) {
    auto num_cols = model->columns();
    
    for (int j = 0; j < num_cols; ++j) {
        if (model->header_text(j) == header) {
            return j;
        }
    }

    return -1;
}

std::vector<std::string> all_headers(Model* model) {
    auto num_cols = model->columns();

    std::vector<std::string> vector;
    vector.reserve(num_cols);
    for (int i = 0; i < num_cols; ++i) {
        vector.push_back(model->header_text(i));
    }

    return vector;
}

}
