//
//  model.hpp
//  ddui-table
//
//  Created by Bartholomew Joyce on 22/03/2018.
//  Copyright © 2018 Bartholomew Joyce All rights reserved.
//

#ifndef ddui_table_model_hpp
#define ddui_table_model_hpp

#include <vector>
#include <string>
#include <ddui/views/Menu>

namespace Table {

struct Model {
    enum RenderCellResult {
        PERFORMED_RENDER,
        USE_DEFAULT_RENDER
    };

    virtual ~Model() = default;

    virtual long ref() = 0; // returns a number that changes
                            // when the underlying data of the
                            // model changes.
    virtual int columns() = 0;
    virtual int rows() = 0;
    virtual const std::string& header_text(int col) = 0;
    virtual const std::string& cell_text(int row, int col) = 0;
    virtual std::vector<int> key() = 0;
    virtual bool cell_editable(int row, int col) {
        // as a default, cells are not editable
        return false;
    };
    virtual void set_cell_text(int row, int col, const std::string& text) = 0;
    virtual RenderCellResult render_cell(int row, int col, bool is_selected) {
        // as a default, we have no custom rendering
        return USE_DEFAULT_RENDER;
    };
};

class BasicModel : public Model {
    public:
        BasicModel();
        BasicModel(std::vector<std::string> headers,
                   std::vector<std::string> key);
        void insert_row(std::vector<std::string> row);
        void replace_content(std::vector<std::string> headers,
                             std::vector<std::vector<std::string>> data);
        bool editable;

        // Implement Model methods
        long ref() {
            return version_count;
        }
        int columns() {
            return headers.size();
        }
        int rows() {
            return data.size();
        }
        const std::string& header_text(int col) {
            return headers[col];
        }
        const std::string& cell_text(int row, int col) {
            return data[row][col];
        }
        std::vector<int> key() {
            return key_;
        }
        bool cell_editable(int row, int col) {
            return editable;
        }
        void set_cell_text(int row, int col, const std::string& text) {
            data[row][col] = text;
            ++version_count;
        }

    private:
        int version_count; // increments when state is changed
        std::vector<std::string> headers;
        std::vector<std::vector<std::string>> data;
        std::vector<int> key_;
};

int get_header_index(Model* model, std::string header);
std::vector<std::string> all_headers(Model* model);

}

#endif
