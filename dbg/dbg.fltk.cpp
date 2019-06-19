#include "dbg.h"

#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table.H>
#include <FL/fl_draw.H>

#include <assert.h>
#include <vector>

#ifdef __GNUC__
#define GG_PACKED_STRUCT struct __attribute__((__packed__))
#elif defined __WATCOMC__
#define GG_PACKED_STRUCT _Packed struct
#elif defined _MSC_VER
#define GG_PACKED_STRUCT __declspec(align(1)) struct
#else
#define GG_PACKED_STRUCT struct
#endif

///////////////////////////////////////////////////////////////////////////////

// Implementation of the debug window using FLTK

///////////////////////////////////////////////////////////////////////////////
// Implements the address/break-toggle/disassembly/machine-code table
class GG_Table : public Fl_Table {
    
    struct Row {
        std::string disassembly;
        unsigned short address;
        unsigned char byte0, byte1, byte2;
        bool br : 1;
        unsigned char byte_len : 2;
        
        inline void write_address(char *buffer) const {
            snprintf(buffer, 7, "0x%0.4X", address);
            buffer[6] = 0;
        }
        
        inline void write_bytes(char *buffer) const {
            snprintf(buffer, 5, "0x%0.2X", byte0);
            const unsigned char bl = byte_len;
            if(bl > 1){
                snprintf(buffer+4, 6, " 0x%0.2X", byte1);
                if(bl > 2){
                    snprintf(buffer+9, 6, " 0x%0.2X", byte2);
                }
            }
        }
    };
    
public:
    
    GG_Table(int a_x, int a_y, int a_w, int a_h)
      : Fl_Table(a_x, a_y, a_w, a_h)
      , selected_row(0){
        m_rows.reserve(64);
    }
    
    void update(const GG_DebuggerUI *dbg, unsigned num_lines){
        assert(dbg != NULL);
        m_rows.resize(num_lines);
        for(unsigned i = 0; i < num_lines; i++){
            const GG_DebuggerUILine *const line = GG_GetDebuggerUILine(dbg, i);
            assert(line != NULL);
            const unsigned address = GG_GetDebuggerUILineAddress(line);
            const char *const text = GG_GetDebuggerUILineText(line);
            
            Row &row = m_rows[i];
            row.address = address;
            row.disassembly = text;
            
            // TODO!
            row.byte0 = 1;
            row.br = 0;
            row.byte_len = 1;
        }
        rows(num_lines);
    }
    
protected:
    
    virtual void draw_cell(TableContext ctx,
        int a_row, int a_col,
        int a_x, int a_y, int a_w, int a_h){
        
        char buffer[40];
        switch(ctx){
            case CONTEXT_STARTPAGE:
                fl_font(FL_COURIER, 14);
                return;
            case CONTEXT_COL_HEADER:
                switch(a_col){
                    case 0: draw_header("Break", a_x, a_y, a_w, a_h); return;
                    case 1: draw_header("Disassembly", a_x, a_y, a_w, a_h); return;
                    case 2: draw_header("Machine Code", a_x, a_y, a_w, a_h); return;
                    default: assert(false && "invalid column"); return;
                }
            case CONTEXT_ROW_HEADER:
                assert((unsigned)a_row < m_rows.size());
                m_rows[a_row].write_address(buffer);
                draw_header(buffer, a_x, a_y, a_w, a_h);
                return;
            case CONTEXT_CELL:
                assert((unsigned)a_row < m_rows.size());
                switch(a_col){
                    case 0:
                        draw_break_cell(m_rows[a_row].br, a_x, a_y, a_w, a_h);
                        return;
                    case 1:
                        draw_cell(m_rows[a_row].disassembly.c_str(), a_x, a_y, a_w, a_h);
                        return;
                    case 2:
                        m_rows[a_row].write_bytes(buffer);
                        draw_cell(buffer, a_x, a_y, a_w, a_h);
                    default: assert(false && "invalid column"); return;
                }
        }
    }
    
private:
    
    static inline void draw_header(const char *text,
        int a_x, int a_y, int a_w, int a_h){
        fl_push_clip(a_x, a_y, a_w, a_h);
        fl_color(FL_BLACK);
        fl_draw(text, a_x, a_y, a_w, a_h, FL_ALIGN_LEFT);
        fl_pop_clip();
    }
    
    static inline void draw_break_cell(bool br,
        int a_x, int a_y, int a_w, int a_h){
        fl_color(FL_WHITE);
        fl_rectf(a_x, a_y, a_w, a_h);
        if(br){
            fl_push_clip(a_x, a_y, a_w, a_h);
            fl_color(FL_RED);
            fl_circle(a_x + 6.0, a_y + 6.0, 8.0);
            fl_pop_clip();
        }
    }
    
    static inline void draw_cell(const char *text,
        int a_x, int a_y, int a_w, int a_h){
        fl_color(FL_WHITE);
        fl_rectf(a_x, a_y, a_w, a_h);
        
        fl_push_clip(a_x, a_y, a_w, a_h);
        fl_color(FL_GRAY0);
        fl_draw(text, a_x, a_y, a_w, a_h, FL_ALIGN_LEFT);
        fl_pop_clip();
    }
    std::vector<Row> m_rows;
    int selected_row;
};

///////////////////////////////////////////////////////////////////////////////

struct GG_DebuggerWindow{
    GG_DebuggerWindow(GG_DebuggerUI *dbg)
      : m_window(600, 400, "Debugger")
      , m_table(50, 50, 400, 300)
      , m_dbg(dbg){
        m_window.show();
    }
    
    ~GG_DebuggerWindow(){
        m_window.clear();
    }
    
    Fl_Window m_window;
    GG_Table m_table;
    GG_DebuggerUI *const m_dbg;
    
    mutable struct x {
        int start, end;
        
        inline bool operator==(GG_DebuggerWindow::x other) const {
            return start == other.start && end == other.end;
        }
        
        inline bool operator!=(GG_DebuggerWindow::x other) const {
            return !(*this == other);
        }
        
    } needed, old_needed;
};

///////////////////////////////////////////////////////////////////////////////

GG_DebuggerWindow* GG_CreateDebuggerWindow(GG_DebuggerUI *dbg){
    return new GG_DebuggerWindow(dbg);
}

///////////////////////////////////////////////////////////////////////////////

void GG_DestroyDebuggerWindow(GG_DebuggerWindow *win){
    delete win;
}

///////////////////////////////////////////////////////////////////////////////

void GG_UpdateDebuggerWindow(struct GG_DebuggerWindow *win){
    win->m_table.update(win->m_dbg, win->needed.end - win->needed.start);
    win->m_window.redraw();
    Fl::check();
}

///////////////////////////////////////////////////////////////////////////////

void GG_PollDebuggerWindow(GG_DebuggerWindow *win){
    Fl::check();
    // TODO: return
    win->m_window.shown();
}

///////////////////////////////////////////////////////////////////////////////

int GG_GetDebuggerWindowNeededLines(const GG_DebuggerWindow *win,
    unsigned *out_start, unsigned *out_end){
    
    if(win->needed != win->old_needed){
        out_start[0] = win->needed.start;
        out_end[0] = win->needed.end;
        win->old_needed = win->needed;
        return 1;
    }
    else{
        return 0;
    }
}
