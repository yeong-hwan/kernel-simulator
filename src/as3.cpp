#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <string>
using namespace std;
namespace fs = std::filesystem;

class process
{
public:
    string name;
    int id;
    int parent_id;
    string state;
    string is_wait;
    int wait_cnt;

    // as3
    int fifo_ctr; // 0 (oldest)

    list<string> physical_memory_list; // 16
    // process_id(page_id)
    // if cow share, init_id

    list<string> virtual_memory_list; // 32
    // page_id

    list<string> page_table_p_loca_list; // 32
    // 해당 virtual memory의 physical memory에서의 위치

    list<string> page_table_rw_list; // 32
    // R or W

    list<int> allocate_info; //
                             // row : idx | col : allocate_id
                             // 0 | 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15

    /*
    4. physical memory:
    |2(0) 2(18) 2(19) 2(20)|1(4) 1(5) 1(6) 1(7)|1(8) 1(9) 1(10) 1(11)|1(12)
    1(13) 1(14) 1(15)|

    5. virtual memory:
    |0 1 2 3|4 5 6 7|8 9 10 11|12 13 14 15|18 19 20 -|- - - -|- - - -|- - - -|

    6. page table:
    |0 - - -|4 5 6 7|8 9 10 11|12 13 14 15|1 2 3 -|- - - -|- - - -|- - - -|
    |W R R R|R R R R|R R R R|R R R R|W W W -|- - - -|- - - -|- - - -|
    */

    list<string> fifo_list;

    process()
    {
        name = "";
        id = 0;
        parent_id = 0;
        state = "";
        is_wait = "";
        wait_cnt = 0;
    }
    process(string n)
    {
        name = n;
        id = 0;
        parent_id = 0;
        state = "New"; // new, ready, waiting, running, terminated
        is_wait = "off";
        wait_cnt = 0;

        // as3 update

        while (physical_memory_list.size() < 16)
        {
            physical_memory_list.push_back("-");
        }

        while (virtual_memory_list.size() < 32)
        {
            virtual_memory_list.push_back("-");
        }

        while (page_table_p_loca_list.size() < 32)
        {
            page_table_p_loca_list.push_back("-");
        }

        while (page_table_rw_list.size() < 32)
        {
            page_table_rw_list.push_back("-");
        }
    }
    void schedule()
    {
        if (!state.compare("Ready"))
        {
            state = "Running";
        }
    }
};

int program_num;
string *program_name;
string *program_command;

queue<string> *split(string str, char Delimiter)
{
    istringstream iss(str);
    string buffer;

    queue<string> *result = new queue<string>;

    while (getline(iss, buffer, Delimiter))
    {
        result->push(buffer);
    }

    return result;
}

int main(int argc, char **argv)
{
    int cycle = 0;
    int process_id = 1;

    string mode = "kernel";
    // boot, system call, schedule, idle, +fault
    string kernel_mode = "";
    string command = "";

    // lists

    list<process *> process_list;
    process *process_now;
    list<process *> ready_queue;
    list<string> waiting_queue;

    int run_remain_cycle = 0;

    // memory_num
    int memory_allocate_num;
    int memory_release_num;
    int memory_read_num;
    int memory_write_num;

    string new_program_name;

    // memory kernel cycle
    int allocate_cycle = 0;
    int release_cycle = 0;
    int read_cycle = 0;
    int write_cycle = 0;

    // allocation_id
    int allocation_id = 0; // 0 ~
                           // memory_allocation 명령어 실행마다 1 증가
                           // 생성 시점에 부모 프로세스로부터 alloc_id 복사해옴

    //
    string page_state = "";

    int page_id = 0;

    // other kernel cycle
    int fork_cycle = 0;
    int exit_cycle = 0;
    int wait_cycle = 0;
    string wait_to_ready = "off";

    string end_trigger = "off";
    process process_terminated;
    process *forked_process;
    process forked_process_save;

    string user_trigger = "off";

    string answer = "";

    // page replace algorithm
    // fifo, lru, lfu, mfu
    string page_replace_algorithm = argv[2];

    string dir_name = argv[1];
    for (const auto &entry : fs::directory_iterator(dir_name))
    {
        program_num++;
    }
    program_name = new string[program_num];
    program_command = new string[program_num];
    int i = 0;
    for (const auto &entry : fs::directory_iterator(dir_name))
    {
        program_name[i] = entry.path().filename().string();
        ifstream file(entry.path());
        string temp_command;
        if (file.is_open())
        {
            string line;
            while (getline(file, line))
            {
                temp_command += line;
                temp_command += "\n";
            }
            file.close();
        }
        program_command[i] = temp_command;
        i++;
    }

    queue<string> command_queue;
    queue<string> *init_command_queue;
    queue<string> *fork_command_queue;

    pair<int, queue<string> *> pid_command;
    pid_command = make_pair(0, &command_queue);

    list<pair<int, queue<string> *>> pid_command_list;
    pid_command_list.push_back(pid_command);

    while (true)
    {
        answer += "[cycle #";
        answer += to_string(cycle);
        answer += "]\n";

        if (cycle == 2)
        {
            mode = "user";
        }

        if (user_trigger == "on")
        {
            mode = "user";
            user_trigger = "off";
        }

        answer += "1. mode: ";
        answer += mode;
        list<process *>::iterator iter;
        list<string>::iterator iter1;
        list<process *>::iterator iter3;

        // added as3
        list<string>::iterator iter_physical;
        list<string>::iterator iter_virtual;
        list<string>::iterator iter_pagetable_p_loca;
        list<string>::iterator iter_pagetable_rw;

        // each process
        for (iter3 = process_list.begin(); iter3 != process_list.end();
             iter3++)
        {
            process *temp = *iter3;
            int now_pid = 0;
            if (temp->wait_cnt != 0)
            {
                temp->wait_cnt -= 1;
                if (temp->wait_cnt == 1)
                {
                    now_pid = temp->id;
                    for (iter1 = waiting_queue.begin();
                         iter1 != waiting_queue.end(); iter1++)
                    {
                        string process_candidate = *iter1;

                        int process_wait_pid = stoi(process_candidate.substr(
                            0, process_candidate.length() - 3));
                        if (process_wait_pid == now_pid)
                        {
                            temp->state = "Ready";
                            waiting_queue.erase(iter1);
                            ready_queue.push_back(temp);
                            break;
                        }
                    }
                }
            }
        }

        // user case
        if (mode == "user")
        {
            // not running
            if (run_remain_cycle == 0)
            {
                list<pair<int, queue<string> *>>::iterator iter2;
                queue<string> *user_command_queue;
                for (iter2 = pid_command_list.begin();
                     iter2 != pid_command_list.end(); iter2++)
                {
                    int pid = iter2->first;
                    user_command_queue = iter2->second;

                    if (process_now->id == pid)
                    {
                        command = (*user_command_queue).front();
                        (*user_command_queue).pop();
                        break;
                    }
                }

                string command_sliced = command.substr(0, 1);

                string command_memory;
                // cout << command;

                if (command_sliced == "m")
                {
                    command_memory = command.substr(7, 3);
                    // answer += command_memory;
                    // break;

                    // allocate [systeml_call]
                    if (command_memory == "all")
                    {
                        memory_allocate_num = stoi(command.substr(16));
                        allocate_cycle = 2;
                        mode = "kernel";
                    }
                    // release [system_call]
                    else if (command_memory == "rel")
                    {
                        memory_release_num = stoi(command.substr(15));
                        release_cycle = 2;

                        mode = "kernel";
                    }

                    // read [user]
                    else if (command_memory == "rea")
                    {
                        memory_read_num = stoi(command.substr(12));
                        read_cycle = 2;
                        // if memory_read_num in physical memory,

                        iter_physical = process_now->physical_memory_list.begin();

                        for (iter_physical = process_now->physical_memory_list.begin();
                             iter_physical != process_now->physical_memory_list.end();
                             iter_physical++)
                        {
                            string page_num = *iter_physical;

                            // delete ")"
                            page_num = page_num.substr(0, page_num.length() - 1);

                            // delete process_id
                            string letter_now = ")";
                            string letter_target = "(";

                            while (letter_now != letter_target)
                            {
                                letter_now = page_num.substr(0, 1);
                                page_num = page_num.substr(1, page_num.length());
                            }
                            if (page_num == to_string(memory_read_num))
                            {
                                read_cycle = 0;
                            }
                        }
                        if (read_cycle == 2)
                        {
                            mode = "kernel";
                        }
                    }
                    // write [user]
                    else if (command_memory == "wri")
                    {
                        memory_write_num = stoi(command.substr(13));

                        write_cycle = 2;

                        // if memory_write_num in physical memory,

                        iter_physical = process_now->physical_memory_list.begin();

                        for (iter_physical = process_now->physical_memory_list.begin();
                             iter_physical != process_now->physical_memory_list.end();
                             iter_physical++)
                        {
                            string page_num = *iter_physical;

                            // delete ")"
                            page_num = page_num.substr(0, page_num.length() - 1);

                            // delete process_id
                            string letter_now = ")";
                            string letter_target = "(";

                            while (letter_now != letter_target)
                            {
                                letter_now = page_num.substr(0, 1);
                                page_num = page_num.substr(1, page_num.length());
                            }
                            if (page_num == to_string(memory_write_num))
                            {
                                write_cycle = 0;
                                // page_state
                                iter_pagetable_rw = process_now->page_table_rw_list.begin();
                                advance(iter_pagetable_rw, memory_write_num);
                                page_state = *iter_pagetable_rw;
                            }
                        }
                        if (page_state == "R")
                        {
                            write_cycle = 2;
                            mode = "kernel";
                        }
                        // page not in physical_memory
                        else if (page_state == "W" && write_cycle != 0)
                        {
                            write_cycle = 1;
                            mode = "kernel";
                        }
                    }
                }

                // fork_and_exec
                else if (command_sliced == "f")
                {
                    new_program_name = command.substr(14);
                    fork_cycle = 2;

                    mode = "kernel";
                }

                // wait
                else if (command_sliced == "w")
                {
                    wait_cycle = 2;

                    mode = "kernel";
                }

                // exit
                else if (command_sliced == "e")
                {
                    exit_cycle = 2;
                    process_now->is_wait = "off";

                    mode = "kernel";
                }
            }
        }
        // kernel case
        else if (mode == "kernel")
        {
            // booting
            if (cycle == 0)
            {
                kernel_mode = "boot";
                command = kernel_mode;

                process *init = new process("init");
                process_now = init;
                process_list.push_back(process_now);

                process_now->id = 1;
                process_now->parent_id = 0;
                process_now->state = "New";

                for (i = 0; i < program_num; i++)
                {
                    if (program_name[i] == process_now->name)
                    {
                        init_command_queue = split(program_command[i], '\n');

                        pid_command =
                            make_pair(process_now->id, init_command_queue);
                        pid_command_list.push_back(pid_command);

                        break;
                    }
                }
            }
            // ready @ cycle 1
            else if (cycle == 1)
            {
                process_now->state = "Ready";

                kernel_mode = "schedule";
                command = kernel_mode;

                process_now->schedule();
            }

            // memory_allocate
            else if (allocate_cycle != 0)
            {
                allocate_cycle -= 1;

                if (allocate_cycle == 1)
                {
                    kernel_mode = "system call";
                    command = kernel_mode;

                    // // EDIT ====================

                    // 1. start index finding
                    int alloc_num;
                    int start_idx = 0; // start_idx
                    int alloc_len = 0;

                    iter_virtual = process_now->virtual_memory_list.begin();
                    int idx_virtual = 0; // 0~31

                    // index finding
                    list<int> idx_list;

                    string pass = "on";

                    alloc_num = memory_allocate_num;

                    for (iter_virtual =
                             process_now->virtual_memory_list.begin();
                         iter_virtual != process_now->virtual_memory_list.end();
                         iter_virtual++)
                    {
                        // empty case
                        if (*iter_virtual == "-")
                        {
                            alloc_len += 1;
                            // empty_cnt += 1;
                        }
                        // not empty
                        else
                        {
                            start_idx = idx_virtual;
                        }

                        if (alloc_len == alloc_num)
                        {
                            break;
                        }

                        idx_virtual += 1;
                    }

                    // target_idx finding ==============
                    int empty_cnt = 0;

                    iter_physical = process_now->physical_memory_list.begin();

                    for (iter_physical =
                             process_now->physical_memory_list.begin();
                         iter_physical !=
                         process_now->physical_memory_list.end();
                         iter_physical++)
                    {
                        if (*iter_physical == "-")
                        {
                            empty_cnt += 1;
                        }
                    }
                    // ================================
                    // fault handler by algorithm
                    if (empty_cnt < memory_allocate_num)
                    {
                        if (page_replace_algorithm == "fifo")
                        {
                            // physical page reallocate
                            int cnt = memory_allocate_num;
                            int v_start_idx = start_idx;
                            while (cnt)
                            {
                                string page_to_delete = process_now->fifo_list.front();
                                process_now->fifo_list.pop_front();

                                // add at fifo_list
                                string new_page;
                                new_page += to_string(process_now->id);
                                new_page += "(";
                                new_page += to_string(page_id);
                                new_page += ")";

                                // replace physical_memory
                                iter_physical = process_now->physical_memory_list.begin();

                                int idx_save = 0;

                                for (iter_physical =
                                         process_now->physical_memory_list.begin();
                                     iter_physical !=
                                     process_now->physical_memory_list.end();
                                     iter_physical++)
                                {

                                    string page_physical = *iter_physical;

                                    // main useful
                                    if (page_to_delete == page_physical)
                                    {
                                        process_now->fifo_list.push_back(new_page);
                                        // replace
                                        process_now->physical_memory_list.insert(
                                            iter_physical, new_page);
                                        process_now->physical_memory_list.erase(
                                            iter_physical);

                                        // virtual page reallocation
                                        iter_virtual = process_now->virtual_memory_list.begin();
                                        int virtual_idx = 0;
                                        int virtual_cnt = alloc_num;

                                        for (iter_virtual = process_now->virtual_memory_list.begin();
                                             iter_virtual != process_now->virtual_memory_list.end();
                                             iter_virtual++)
                                        {
                                            if (virtual_idx > v_start_idx && virtual_cnt > 0)
                                            {
                                                process_now->virtual_memory_list.insert(
                                                    iter_virtual, to_string(page_id));
                                                process_now->virtual_memory_list.erase(
                                                    iter_virtual);
                                                // cout << "\n"
                                                virtual_cnt -= 1;
                                                break;
                                            }
                                            virtual_idx += 1;
                                        }
                                        // virtual_idx += 1;
                                        v_start_idx += 1;

                                        page_id += 1;
                                        break;
                                    }

                                    idx_list.push_back(idx_save);
                                    idx_save += 1;
                                }

                                cnt -= 1;
                                // cout << cnt;
                            }
                        }
                        else if (page_replace_algorithm == "lfu")
                        {
                            ;
                        }
                        else if (page_replace_algorithm == "lru")
                        {
                            ;
                        }
                        else if (page_replace_algorithm == "mfu")
                        {
                            ;
                        }

                        pass = "off";
                    }
                    // remain spare
                    else
                    {
                        pass = "on";
                    }
                    // cout << "\n" << empty_cnt << "\n";

                    list<int> idx_to_delete;

                    // 2. page allocation
                    // data for 3.
                    int page_id_start = page_id;
                    int page_id_end = page_id_start + alloc_num;

                    list<int> page_idx_list_1; // for 4. page table
                    list<int> page_idx_list_2;
                    int alloc_idx = 0;

                    //
                    iter_virtual = process_now->virtual_memory_list.begin();
                    idx_virtual = 0; // 0~31 (reset)

                    for (iter_virtual =
                             process_now->virtual_memory_list.begin();
                         iter_virtual != process_now->virtual_memory_list.end();
                         iter_virtual++)
                    {

                        if (alloc_num == 0)
                        {
                            break;
                        }
                        if (idx_virtual < alloc_idx)
                        {
                            idx_virtual += 1;
                            continue;
                        }
                        //
                        else
                        {
                            alloc_num -= 1;
                            // replace
                            process_now->virtual_memory_list.insert(
                                iter_virtual, to_string(page_id));
                            process_now->virtual_memory_list.erase(
                                iter_virtual);

                            page_id += 1;
                        }
                        page_idx_list_1.push_back(idx_virtual);
                        page_idx_list_2.push_back(idx_virtual);
                        process_now->allocate_info.push_back(alloc_idx);

                        idx_virtual += 1;
                        alloc_idx += 1;
                    }
                    process_now->allocate_info.push_back(allocation_id);
                    allocation_id += 1;

                    // =============================
                    // list<int>::iterator iter_alloc;
                    // iter_alloc = process_now->allocate_info.begin();
                    // int idx_alloc = 0; //

                    // for (iter_alloc =
                    //          process_now->allocate_info.begin();
                    //      iter_alloc !=
                    //      process_now->allocate_info.end();
                    //      iter_alloc++)
                    // {
                    //     if (idx_alloc % 4 == 0)
                    //     {
                    //         cout << "|";
                    //         cout << *iter_alloc;
                    //         cout << " ";
                    //     }
                    //     else if ((idx_alloc + 1) % 4 == 0)
                    //     {
                    //         cout << *iter_alloc;
                    //     }
                    //     else
                    //     {
                    //         cout << *iter_alloc;
                    //         cout << " ";
                    //     }

                    //     idx_alloc += 1;
                    // }

                    // cout << "|\n\n";
                    // ======================================

                    // // EDIT ====================

                    // 3. frame allocation

                    alloc_num = memory_allocate_num;
                    list<int> frame_idx_list; // for 4. page table

                    iter_physical = process_now->physical_memory_list.begin();
                    int idx_physical = 0; // 0~15

                    int page_id_now = page_id_start;

                    for (iter_physical =
                             process_now->physical_memory_list.begin();
                         iter_physical !=
                         process_now->physical_memory_list.end();
                         iter_physical++)
                    {
                        // not useful
                        if (alloc_num == 0)
                        {
                            // cout << "break by alloc";
                            break;
                        }

                        // main useful
                        if (*iter_physical == "-")
                        {
                            string now_string;
                            now_string += to_string(process_now->id);

                            now_string += "(";
                            now_string += to_string(page_id_now);
                            now_string += ")";

                            // cout << now_string << "\n\n";
                            process_now->fifo_list.push_back(now_string);

                            // replace
                            process_now->physical_memory_list.insert(
                                iter_physical, now_string);
                            process_now->physical_memory_list.erase(
                                iter_physical);

                            page_id_now += 1;
                            alloc_num -= 1;

                            // not useful
                            if (page_id_now > page_id_end)
                            {
                                // cout << "break by idx";
                                break;
                            }
                        }

                        frame_idx_list.push_back(idx_physical);
                        idx_physical += 1;
                    }

                    // 4. page table  update
                    // 4-1. page_location
                    iter_pagetable_p_loca = process_now->page_table_p_loca_list.begin();
                    int idx_pt_p_loca = 0; // 0~31

                    for (iter_pagetable_p_loca = process_now->page_table_p_loca_list.begin();
                         iter_pagetable_p_loca != process_now->page_table_p_loca_list.end();
                         iter_pagetable_p_loca++)
                    {
                        int idx_frame = frame_idx_list.front();
                        int idx_virtual = page_idx_list_1.front();

                        // cout << "idx_p_loca: " << idx_pt_p_loca << "\n";
                        // cout << "idx_frame: " << idx_frame << "\n";
                        // cout << "frame_idx_list(size): " << frame_idx_list.size() << "\n";
                        // cout << "idx_virtual: " << idx_virtual << "\n";
                        // allocate
                        if (frame_idx_list.size() == 0)
                        {
                            break;
                        }
                        if (idx_pt_p_loca == idx_virtual)
                        {
                            frame_idx_list.pop_front();
                            page_idx_list_1.pop_front();

                            // idx_frame
                            process_now->page_table_p_loca_list.insert(
                                iter_pagetable_p_loca, to_string(idx_frame));
                            process_now->page_table_p_loca_list.erase(
                                iter_pagetable_p_loca);

                            // =============================
                            // iter_virtual = process_now->page_table_p_loca_list.begin();
                            // idx_virtual = 0; // 0~31

                            // for (iter_virtual =
                            //          process_now->page_table_p_loca_list.begin();
                            //      iter_virtual !=
                            //      process_now->page_table_p_loca_list.end();
                            //      iter_virtual++)
                            // {
                            //     if (idx_virtual % 4 == 0)
                            //     {
                            //         cout << "|";
                            //         cout << *iter_virtual;
                            //         cout << " ";
                            //     }
                            //     else if ((idx_virtual + 1) % 4 == 0)
                            //     {
                            //         cout << *iter_virtual;
                            //     }
                            //     else
                            //     {
                            //         cout << *iter_virtual;
                            //         cout << " ";
                            //     }

                            //     idx_virtual += 1;
                            // }

                            // cout << "|\n\n";
                            // ======================================
                        }
                        // // empty
                        else
                        {
                            process_now->page_table_p_loca_list.insert(
                                iter_pagetable_p_loca, "-");
                            process_now->page_table_p_loca_list.erase(
                                iter_pagetable_p_loca);
                        }
                        idx_pt_p_loca += 1;
                    }

                    // 4-2. rw info
                    iter_pagetable_rw = process_now->page_table_rw_list.begin();
                    int idx_pt_rw = 0;

                    for (iter_pagetable_rw = process_now->page_table_rw_list.begin();
                         iter_pagetable_rw != process_now->page_table_rw_list.end();
                         iter_pagetable_rw++)
                    {
                        int idx_virtual = page_idx_list_2.front();

                        if (idx_pt_rw == idx_virtual)
                        {
                            page_idx_list_2.pop_front();

                            process_now->page_table_rw_list.insert(
                                iter_pagetable_rw, "W");
                            process_now->page_table_rw_list.erase(
                                iter_pagetable_rw);
                        }

                        idx_pt_rw += 1;
                    }

                    // answer +=
                    // case=========================================

                    // iter_virtual =
                    // process_now->virtual_memory_list.begin(); idx_virtual
                    // = 0;  // 0~31

                    // for (iter_virtual =
                    //          process_now->virtual_memory_list.begin();
                    //      iter_virtual !=
                    //      process_now->virtual_memory_list.end();
                    //      iter_virtual++) {
                    //     if (idx_virtual % 4 == 0) {
                    //         answer += "|";
                    //         answer += *iter_virtual;
                    //         answer += " ";
                    //     } else if ((idx_virtual + 1) % 4 == 0) {
                    //         answer += *iter_virtual;
                    //     } else {
                    //         answer += *iter_virtual;
                    //         answer += " ";
                    //     }

                    //     idx_virtual += 1;
                    // }

                    // cout case
                    // ============================================

                    // iter_virtual = process_now->virtual_memory_list.begin();
                    // idx_virtual = 0;  // 0~31

                    // for (iter_virtual =
                    //          process_now->virtual_memory_list.begin();
                    //      iter_virtual !=
                    //      process_now->virtual_memory_list.end();
                    //      iter_virtual++) {
                    //     if (idx_virtual % 4 == 0) {
                    //         cout << "|";
                    //         cout << *iter_virtual;
                    //         cout << " ";
                    //     } else if ((idx_virtual + 1) % 4 == 0) {
                    //         cout << *iter_virtual;
                    //     } else {
                    //         cout << *iter_virtual;
                    //         cout << " ";
                    //     }

                    //     idx_virtual += 1;
                    // }

                    // cout << "|";

                    // ==============================================
                    process_now->state = "Ready";
                    ready_queue.push_back(process_now);
                }
                // -------------

                //
                else if (allocate_cycle == 0)
                {
                    // break;
                    kernel_mode = "schedule";
                    command = kernel_mode;

                    process_now = ready_queue.front();
                    ready_queue.pop_front();

                    process_now->schedule();
                    user_trigger = "on";
                    // break;
                }
            }

            // memory_release
            else if (release_cycle != 0)
            {
                release_cycle -= 1;

                if (release_cycle == 1)
                {
                    kernel_mode = "system call";
                    command = kernel_mode;

                    // memory_release !!!!!!!
                }
                //
                else if (release_cycle == 0)
                {
                    kernel_mode = "schedule";
                    command = kernel_mode;

                    process_now = ready_queue.front();
                    ready_queue.pop_front();

                    process_now->schedule();
                    user_trigger = "on";
                }
            }

            // memory_read
            else if (read_cycle != 0)
            {
                read_cycle -= 1;

                // page fault handler
                if (read_cycle == 1)
                {
                    kernel_mode = "fault";
                    command = kernel_mode;

                    // page algorithm
                    // fifo
                    if (page_replace_algorithm == "fifo")
                    {
                        int alloc_num;
                        alloc_num = memory_allocate_num; // cnt

                        while (alloc_num > 0)
                        {
                            string page_to_delete = process_now->fifo_list.front();
                            process_now->fifo_list.pop_front();

                            // add at fifo_list
                            string new_page;
                            new_page += to_string(process_now->id);
                            new_page += "(";
                            new_page += to_string(memory_write_num);
                            new_page += ")";

                            // replace physical_memory
                            iter_physical = process_now->physical_memory_list.begin();

                            // cout << *iter_physical << "\n\n\n\n\n";

                            for (iter_physical =
                                     process_now->physical_memory_list.begin();
                                 iter_physical !=
                                 process_now->physical_memory_list.end();
                                 iter_physical++)
                            {

                                string page_num = *iter_physical;

                                // delete ")"
                                page_num = page_num.substr(0, page_num.length() - 1);

                                // delete process_id
                                string letter_now = ")";
                                string letter_target = "(";

                                while (letter_now != letter_target)
                                {
                                    letter_now = page_num.substr(0, 1);
                                    page_num = page_num.substr(1, page_num.length());
                                }

                                // main useful

                                if (page_num == to_string(memory_write_num))
                                {

                                    process_now->fifo_list.push_back(new_page);

                                    // replace
                                    process_now->physical_memory_list.insert(
                                        iter_physical, new_page);
                                    process_now->physical_memory_list.erase(
                                        iter_physical);
                                }
                            }

                            alloc_num -= 1;
                        }
                    }

                    // lfu
                    else if (page_replace_algorithm == "lfu")
                    {
                        ; // someting
                    }

                    // lru
                    else if (page_replace_algorithm == "lru")
                    {
                        ; // someting
                    }

                    // mfu
                    else if (page_replace_algorithm == "mfu")
                    {
                        ; // someting
                    }

                    process_now->state = "Ready";
                    ready_queue.push_back(process_now);
                }
                // schedule
                else if (read_cycle == 0)
                {
                    kernel_mode = "schedule";
                    command = kernel_mode;

                    process_now->schedule();
                    user_trigger = "on";
                }
            }
            // memory_write
            else if (write_cycle != 0)
            {
                write_cycle -= 1;
                if (page_state == "R")
                {
                    if (write_cycle == 1)
                    {
                        kernel_mode = "fault";
                        command = kernel_mode;

                        if (page_replace_algorithm == "fifo")
                        {
                            string page_to_delete = process_now->fifo_list.front();
                            process_now->fifo_list.pop_front();

                            // add at fifo_list
                            string new_page;
                            new_page += to_string(process_now->id);
                            new_page += "(";
                            new_page += to_string(memory_write_num);
                            new_page += ")";

                            // replace physical_memory
                            iter_physical = process_now->physical_memory_list.begin();

                            for (iter_physical =
                                     process_now->physical_memory_list.begin();
                                 iter_physical !=
                                 process_now->physical_memory_list.end();
                                 iter_physical++)
                            {

                                string page_num = *iter_physical;

                                // delete ")"
                                page_num = page_num.substr(0, page_num.length() - 1);

                                // delete process_id
                                string letter_now = ")";
                                string letter_target = "(";

                                while (letter_now != letter_target)
                                {
                                    letter_now = page_num.substr(0, 1);
                                    page_num = page_num.substr(1, page_num.length());
                                }

                                // main useful

                                if (page_num == to_string(memory_write_num))
                                {

                                    process_now->fifo_list.push_back(new_page);

                                    // replace
                                    process_now->physical_memory_list.insert(
                                        iter_physical, new_page);
                                    process_now->physical_memory_list.erase(
                                        iter_physical);
                                }
                            }

                            // replace page_table(p_loca, rw)
                            int idx_link = 0;
                            iter_pagetable_p_loca = process_now->page_table_p_loca_list.begin();

                            for (iter_pagetable_p_loca = process_now->page_table_p_loca_list.begin();
                                 iter_pagetable_p_loca != process_now->page_table_p_loca_list.end();
                                 iter_pagetable_p_loca++)
                            {
                                // int idx_frame = frame_idx_list.front();
                                // int idx_virtual = page_idx_list_1.front();

                                if (*iter_pagetable_p_loca == to_string(memory_write_num))
                                {
                                    // idx_frame
                                    process_now->page_table_p_loca_list.insert(
                                        iter_pagetable_p_loca, to_string(memory_write_num));
                                    process_now->page_table_p_loca_list.erase(
                                        iter_pagetable_p_loca);

                                    break;
                                }
                                idx_link += 1;
                            }

                            iter_pagetable_rw = process_now->page_table_rw_list.begin();
                            int idx_pt_rw = 0;

                            for (iter_pagetable_rw = process_now->page_table_rw_list.begin();
                                 iter_pagetable_rw != process_now->page_table_rw_list.end();
                                 iter_pagetable_rw++)
                            {

                                if (idx_link == idx_pt_rw)
                                {

                                    process_now->page_table_rw_list.insert(
                                        iter_pagetable_rw, "W");
                                    process_now->page_table_rw_list.erase(
                                        iter_pagetable_rw);
                                }

                                idx_pt_rw += 1;
                            }
                        }

                        // lfu
                        else if (page_replace_algorithm == "lfu")
                        {
                            ; // someting
                        }

                        // lru
                        else if (page_replace_algorithm == "lru")
                        {
                            ; // someting
                        }

                        // mfu
                        else if (page_replace_algorithm == "mfu")
                        {
                            ; // someting
                        }

                        // =======================
                        // cout << process_now->physical_memory_list.size() << "\n";
                        // cout << process_now->fifo_list.size() << "\n";
                        // list<string>::iterator iter_fifo;
                        // iter_fifo = process_now->fifo_list.begin();
                        // int idx_fifo = 0; // 0~31\


                        // for (iter_fifo =
                        //          process_now->fifo_list.begin();
                        //      iter_fifo !=
                        //      process_now->fifo_list.end();
                        //      iter_fifo++)
                        // {
                        //     if (idx_fifo % 4 == 0)
                        //     {
                        //         cout << "|";
                        //         cout << *iter_fifo;
                        //         cout << " ";
                        //     }
                        //     else if ((idx_fifo + 1) % 4 == 0)
                        //     {
                        //         cout << *iter_fifo;
                        //     }
                        //     else
                        //     {
                        //         cout << *iter_fifo;
                        //         cout << " ";
                        //     }

                        //     idx_fifo += 1;
                        // }

                        // cout << "|\n\n";

                        // =======================

                        process_now->state = "Ready";
                        ready_queue.push_back(process_now);

                        // process_now = ready_queue.front();
                        // ready_queue.pop_front();
                    }
                    else if (write_cycle == 0)
                    {
                        kernel_mode = "schedule";
                        command = kernel_mode;

                        process_now->schedule();
                        user_trigger = "on";
                    }
                }
                else if (page_state == "W")
                {
                    // same as page_fault_handler
                }
            }

            // fork
            else if (fork_cycle != 0)
            {
                fork_cycle -= 1;

                if (fork_cycle == 1)
                {
                    kernel_mode = "system call";
                    command = kernel_mode;

                    process *new_forked_process = new process(new_program_name);
                    process_list.push_back(new_forked_process);
                    new_forked_process->state = "New";

                    new_forked_process->parent_id = process_now->id;

                    process_id += 1;
                    new_forked_process->id = process_id;

                    process_now->state = "Ready";
                    ready_queue.push_back(process_now);
                    forked_process = new_forked_process;
                    for (i = 0; i < program_num; i++)
                    {
                        if (program_name[i] == forked_process->name)
                        {
                            fork_command_queue =
                                split(program_command[i], '\n');

                            pid_command = make_pair(forked_process->id,
                                                    fork_command_queue);
                            pid_command_list.push_back(pid_command);

                            break;
                        }
                    }
                }
                //
                else if (fork_cycle == 0)
                {
                    kernel_mode = "schedule";
                    command = kernel_mode;

                    // list<string>::iterator iter_test;
                    // iter_test = p_m_l_parent.begin();

                    // for (iter_test = p_m_l_parent.begin();
                    //      iter_test != p_m_l_parent.end();
                    //      iter_test++)
                    // {
                    //     cout << *iter_test << "\n";
                    // }

                    process_now = ready_queue.front();

                    ready_queue.pop_front();
                    process_now->schedule();
                    ready_queue.push_back(forked_process);

                    // as3 update =================
                    // process_now = parent
                    list<string> p_m_l_parent = process_now->physical_memory_list;
                    list<string> v_m_l_parent = process_now->virtual_memory_list;
                    list<string> p_t_p_l_parent = process_now->page_table_p_loca_list;
                    list<string> p_t_rw_l_parent = process_now->page_table_rw_list;
                    list<int> allocate_info_parent = process_now->allocate_info;
                    list<string> fifo_list_parent = process_now->fifo_list;

                    // forked_process = child
                    forked_process->physical_memory_list = p_m_l_parent;
                    forked_process->virtual_memory_list = v_m_l_parent;
                    forked_process->page_table_p_loca_list = p_t_p_l_parent;
                    forked_process->fifo_list = fifo_list_parent;

                    // 기존 W 면 다 R로 교체
                    forked_process->page_table_rw_list = p_t_rw_l_parent;

                    list<string>::iterator iter_wtor;

                    iter_wtor = forked_process->page_table_rw_list.begin();

                    for (iter_wtor = forked_process->page_table_rw_list.begin();
                         iter_wtor != forked_process->page_table_rw_list.end();
                         iter_wtor++)
                    {
                        if (*iter_wtor == "W")
                        {
                            forked_process->page_table_rw_list.insert(
                                iter_wtor, "R");
                            forked_process->page_table_rw_list.erase(
                                iter_wtor);
                        }
                    }

                    forked_process->allocate_info = allocate_info_parent;
                    // ============================

                    forked_process->state = "Ready";

                    user_trigger = "on";
                }
            }
            // wait
            else if (wait_cycle != 0)
            {
                if (wait_cycle == 2)
                {
                    kernel_mode = "system call";
                    command = kernel_mode;

                    string wait_trigger = "on";

                    process_now->state = "Ready";

                    for (iter3 = process_list.begin();
                         iter3 != process_list.end(); iter3++)
                    {
                        process *temp = *iter3;
                        if (temp->parent_id == process_now->id)
                        {
                            if (temp->state == "dead" or
                                temp->state == "Terminated")
                            {
                                process_now->state = "Waiting";
                                wait_trigger = "on";

                                break;
                            }
                        }
                    }
                    if (wait_trigger == "on")
                    {
                        string waiting_msg = "";
                        waiting_msg += to_string(process_now->id);
                        waiting_msg += "(W)";

                        waiting_queue.push_back(waiting_msg);
                    }
                    wait_cycle -= 1;
                }
                else
                {
                    if (ready_queue.size() != 0)
                    {
                        kernel_mode = "schedule";
                        command = kernel_mode;

                        process_now = ready_queue.front();
                        ready_queue.pop_front();

                        process_now->schedule();
                        user_trigger = "on";
                    }
                    else
                    {
                        kernel_mode = "idle";
                        command = kernel_mode;
                    }

                    wait_cycle -= 1;
                }
            }
            // exit
            else if (exit_cycle != 0)
            {
                if (exit_cycle == 2)
                {
                    exit_cycle -= 1;

                    kernel_mode = "system call";
                    command = kernel_mode;

                    process_now->state = "Terminated";
                    process_terminated = *process_now;
                    int parent_id_now = process_now->parent_id;

                    if (process_now->id == 1)
                    {
                        end_trigger = "on";
                    }
                    else
                    {
                        for (iter1 = waiting_queue.begin();
                             iter1 != waiting_queue.end(); iter1++)
                        {
                            string wait_process_full = *iter1;
                            int wait_process = stoi(wait_process_full.substr(
                                0, wait_process_full.length() - 3));

                            if (wait_process == parent_id_now)
                            {
                                // use like pop
                                string process_now_iter = *iter1;
                                int process_now_id =
                                    stoi(process_now_iter.substr(
                                        0, process_now_iter.length() - 3));
                                waiting_queue.erase(iter1);

                                for (iter3 = process_list.begin();
                                     iter3 != process_list.end(); iter3++)
                                {
                                    process *temp = *iter3;
                                    if (temp->id == process_now_id)
                                    {
                                        process_now = temp;
                                        break;
                                    }
                                }
                                process_now->state = "Ready";
                                ready_queue.push_back(process_now);
                                break;
                            }
                        }
                    }
                }
                else if (exit_cycle == 1)
                {
                    exit_cycle -= 1;

                    if (ready_queue.size() != 0)
                    {
                        kernel_mode = "schedule";
                        command = kernel_mode;

                        process_now = ready_queue.front();
                        ready_queue.pop_front();

                        process_now->schedule();
                        user_trigger = "on";
                    }
                    else
                    {
                        kernel_mode = "idle";
                        command = kernel_mode;
                    }
                }
            }
        }

        // command
        answer += "\n2. command: ";
        answer += command;
        answer + "\n";

        // running
        string running = "none";

        if (process_now->state == "Running")
        {
            running = to_string(process_now->id);
            running += "(";
            running += process_now->name;
            running += ", ";
            running += to_string(process_now->parent_id);
            running += ")";
        }

        answer += "\n3. running: ";
        answer += running;
        answer += "\n";

        // // ready
        // string ready = " none";

        // if (ready_queue.size() != 0) {
        //     ready = "";

        //     for (iter = ready_queue.begin(); iter != ready_queue.end();
        //          iter++) {
        //         process *temp = *iter;
        //         ready += " ";
        //         ready += to_string(temp->id);
        //     }
        // }

        // answer += "4. ready:";
        // answer += ready;
        // answer += "\n";

        // // waiting
        // string waiting = " none";

        // if (waiting_queue.size() != 0) {
        //     waiting = "";
        //     for (iter1 = waiting_queue.begin(); iter1 !=
        //     waiting_queue.end();
        //          iter1++) {
        //         waiting += " ";
        //         waiting += *iter1;
        //     }
        // }

        // answer += "5. waiting:";
        // answer += waiting;
        // answer += "\n";

        // string new_str = "none";
        // for (iter3 = process_list.begin(); iter3 != process_list.end();
        //      iter3++) {
        //     process *temp = *iter3;
        //     if (temp->state == "New") {
        //         // debug

        //         new_str = to_string(temp->id);
        //         new_str += "(";
        //         new_str += temp->name;
        //         new_str += ", ";
        //         new_str += to_string(temp->parent_id);
        //         new_str += ")";
        //     }
        //     // program_command[i] 가공 -> command_queue
        // }
        // answer += "6. new: ";
        // answer += new_str;
        // answer += "\n";

        // // terminated
        // string terminated;
        // if (process_terminated.id == 0) {
        //     terminated = "none";

        //     answer += "7. terminated: ";
        //     answer += terminated;
        //     answer += "\n";
        // } else if (process_terminated.state == "Terminated") {
        //     terminated += to_string(process_terminated.id);
        //     terminated += "(";
        //     terminated += process_terminated.name;
        //     terminated += ", ";
        //     terminated += to_string(process_terminated.parent_id);
        //     terminated += ")";

        //     answer += "7. terminated: ";
        //     answer += terminated;
        //     answer += "\n";

        //     process_terminated.state = "dead";
        // } else {
        //     terminated = "none";

        //     answer += "7. terminated: ";
        //     answer += terminated;
        //     answer += "\n";
        // }
        // answer += "\n";

        // ---------------------------------------------------------------------------------------
        // physical memory
        answer += "4. physical memory:\n";
        // cout << process_now->id;

        iter_physical = process_now->physical_memory_list.begin();
        int idx_physical = 0; // 0~15

        for (iter_physical = process_now->physical_memory_list.begin();
             iter_physical != process_now->physical_memory_list.end();
             iter_physical++)
        {
            if (idx_physical % 4 == 0)
            {
                answer += "|";
                answer += *iter_physical;
                answer += " ";
            }
            else if ((idx_physical + 1) % 4 == 0)
            {
                answer += *iter_physical;
            }
            else
            {
                answer += *iter_physical;
                answer += " ";
            }
            // cout << *iter_physical << "\n";

            idx_physical += 1;
        }
        answer += "|\n";

        if (command != "boot" && command != "system call" &&
            command != "fault")
        {
            // virtual memory
            answer += "5. virtual memory:\n";

            iter_virtual = process_now->virtual_memory_list.begin();
            int idx_virtual = 0; // 0~31

            for (iter_virtual = process_now->virtual_memory_list.begin();
                 iter_virtual != process_now->virtual_memory_list.end();
                 iter_virtual++)
            {
                if (idx_virtual % 4 == 0)
                {
                    answer += "|";
                    answer += *iter_virtual;
                    answer += " ";
                }
                else if ((idx_virtual + 1) % 4 == 0)
                {
                    answer += *iter_virtual;
                }
                else
                {
                    answer += *iter_virtual;
                    answer += " ";
                }

                idx_virtual += 1;
            }
            answer += "|\n";

            // page table
            answer += "6. page table:\n";

            // page_table_p_loca
            iter_pagetable_p_loca = process_now->page_table_p_loca_list.begin();
            int idx_pagetable_p_loca = 0; // 0~31

            for (iter_pagetable_p_loca =
                     process_now->page_table_p_loca_list.begin();
                 iter_pagetable_p_loca !=
                 process_now->page_table_p_loca_list.end();
                 iter_pagetable_p_loca++)
            {
                if (idx_pagetable_p_loca % 4 == 0)
                {
                    answer += "|";
                    answer += *iter_pagetable_p_loca;
                    answer += " ";
                }
                else if ((idx_pagetable_p_loca + 1) % 4 == 0)
                {
                    answer += *iter_pagetable_p_loca;
                }
                else
                {
                    answer += *iter_pagetable_p_loca;
                    answer += " ";
                }

                idx_pagetable_p_loca += 1;
            }
            answer += "|\n";

            // page_table_rw
            iter_pagetable_rw = process_now->page_table_rw_list.begin();
            int idx_pagetable_rw = 0; // 0~31

            for (iter_pagetable_rw = process_now->page_table_rw_list.begin();
                 iter_pagetable_rw != process_now->page_table_rw_list.end();
                 iter_pagetable_rw++)
            {
                if (idx_pagetable_rw % 4 == 0)
                {
                    answer += "|";
                    answer += *iter_pagetable_rw;
                    answer += " ";
                }
                else if ((idx_pagetable_rw + 1) % 4 == 0)
                {
                    answer += *iter_pagetable_rw;
                }
                else
                {
                    answer += *iter_pagetable_rw;
                    answer += " ";
                }

                idx_pagetable_rw += 1;
            }
            answer += "|\n";
        }

        answer += "\n";

        // end trigger
        if (end_trigger == "on")
        {
            break;
        }

        cycle += 1;
    }
    string result_out_dir = "";

    result_out_dir = fs::path(dir_name).parent_path();

    string file_name = "result";
    fs::path file_path = fs::path(dir_name).parent_path() / file_name;
    fs::create_directories(file_path.parent_path());
    ofstream writeFile(file_path);

    if (writeFile.is_open())
    {
        writeFile << answer;
    }

    writeFile.close();
    return 0;
}