#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
using namespace std;
namespace fs = filesystem;

class process
{
  public:
    string name;
    int id;
    int parent_id;
    string state;
    string is_wait;
    int wait_cnt;

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
    }
    void schedule()
    {
        if (!state.compare("Ready"))
        {
            state = "Running";
        }
    }
};

int main(int argc, char **argv)
{
    int cycle = 0;
    int process_id = 1;

    string mode = "kernel";
    // boot, system call, schedule, idle
    string kernel_mode = "";
    string command = "";

    list<process> process_list;
    process process_now;
    list<process> ready_queue;
    list<string> waiting_queue;

    int run_reamin_cycle = 0;
    int sleep_remain_cycle = 0;
    int sleep_case = 0;

    string new_program_name;

    int fork_cycle = 0;
    int exit_cycle = 0;
    int wait_cycle = 0;
    string wait_to_ready = "off";

    string end_trigger = "off";
    string process_terminated = "";

    string user_trigger = "off";

    string answer = "";

    string dir_name = argv[1];
    for (const auto &entry : fs::directory_iterator(dir_name))
    {
        if (entry.path().filename().string() == "init")
        {
            ifstream file(entry.path());
            if (file.is_open())
            {
                string line;
                while (getline(file, line))
                {
                    cout << line << '\n';
                }
                file.close();
            }
        }
    }

    while (true)
    {
        answer += "[cycle #";
        answer += cycle;
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
        /*
                string str1 = "abcde";

                cout << str1.substr(str1.length() - 1) << endl;  // e
                cout << str1.substr(str1.length() - 2) << endl;  // de
                cout << str1.substr(str1.length() - 3) << endl;  // cde
        */
        answer += "1. mode: ";
        answer += mode;
        list<process>::iterator iter;
        list<string>::iterator iter1;
        // each process
        for (iter = process_list.begin(); iter != process_list.end(); iter++)
        {
            int now_pid = 0;
            if (iter->wait_cnt != 0)
            {
                iter->wait_cnt -= 1;
                if (iter->wait_cnt == 1)
                {
                    now_pid = iter->id;
                    for (iter1 = waiting_queue.begin(); iter1 != waiting_queue.end(); iter1++)
                    {
                        string process_candidate = *iter1;
                        int process_wait_pid = stoi(process_candidate.substr(process_candidate.length() - 3));
                        if (process_wait_pid == now_pid)
                        {
                            iter->state = "Ready";
                            waiting_queue.erase(iter1);
                            ready_queue.push_back(*iter);
                            break;
                        }
                    }
                }
            }
        }

        // user case(user case imp need)
        if (mode == "user")
        {
            // not running
            if (run_reamin_cycle == 0)
            {
                // input implementation need

                // run
                if ()
                {
                }

                // sleep
                else if ()
                {
                }

                // fork_and_exec
                else if ()
                {
                }

                // wait
                else if ()
                {
                }

                // exit
                else if ()
                {
                }
            }
            // if running
            else if (run_reamin_cycle != 0)
            {
                run_reamin_cycle -= 1;
            }
        }
        // kernel case
        else if (mode == "kernel")
        {
            if (cycle == 0)
            {
                kernel_mode = "boot";
                command = kernel_mode;

                process init = process("init");
                process_now = init;
                process_list.push_back(process_now);

                init.id = 1;
                init.parent_id = 0;
                init.state = "New";
            }
            //
            else if (cycle == 1)
            {
                // p_line 141
                process_now.state = "Ready";

                kernel_mode = "schedule";
                command = kernel_mode;

                process_now.schedule();
            }
            // fork (implement need!!!!!!)
            else if (fork_cycle != 0)
            {
                fork_cycle -= 1;

                if (fork_cycle == 1)
                {
                    kernel_mode = "system_call";
                    command = kernel_mode;
                }
                //
                else if (fork_cycle == 0)
                {
                    kernel_mode = "schedule";
                    command = kernel_mode;

                    // p_line 172
                }
            }
            // sleep
            if (process_now.is_wait == "on")
            {
                // sleep == 1
                if (sleep_case == 1)
                {
                    if (process_now.wait_cnt == 1)
                    {
                        kernel_mode == "system call";
                        command = kernel_mode;

                        process_now.state = "Waiting";
                        process_now.state = "Ready";
                        ready_queue.push_back(process_now);
                    }
                    else
                    {
                        kernel_mode = "schedule";
                        command = kernel_mode;

                        // process_now = ready_queue.pop(0)
                        process_now = ready_queue.front();
                        ready_queue.pop_front();

                        process_now.schedule();
                        user_trigger = "on";

                        sleep_case = 0;
                        process_now.is_wait = "off";
                    }
                }
                // sleep == 2
                else if (sleep_case == 2)
                {
                    if (process_now.wait_cnt == 2)
                    {
                        kernel_mode == " system call";
                        command = kernel_mode;

                        process_now.state = "Waiting";

                        string waiting_msg = "";
                        waiting_msg += process_now.id;
                        waiting_msg += "(S)";

                        waiting_queue.push_back(waiting_msg);
                    }
                    else if (process_now.wait_cnt == 1)
                    {
                        kernel_mode == "schedule";
                        command = kernel_mode;

                        // process_now = ready_queue.pop(0)
                        process_now = ready_queue.front();
                        ready_queue.pop_front();

                        process_now.schedule();
                        user_trigger = "on";

                        sleep_case = 0;
                        process_now.wait_cnt = 0;
                        process_now.is_wait = "off";
                    }
                }

                // sleep >= 3
                else if (sleep_case >= 3)
                {
                    // first
                    if (process_now.wait_cnt == sleep_case)
                    {
                        kernel_mode = "system call";
                        command = kernel_mode;

                        string waiting_msg = "";
                        waiting_msg += process_now.id;
                        waiting_msg += "(S)";

                        waiting_queue.push_back(waiting_msg);

                        process_now.state = "Waiting";
                    }
                    // last
                    else if (process_now.wait_cnt == 0)
                    {
                        kernel_mode = "schdule";
                        command = kernel_mode;

                        string process_now_idx;
                        process_now_idx = waiting_queue.back();
                        waiting_queue.pop_back();

                        int process_idx_int = stoi(process_now_idx.substr(process_now_idx.length() - 3));

                        // need 245!!!!!!!!!!!!!!!!!!!!!!11

                        process_now.state = "Ready";
                        ready_queue.push_back(process_now);

                        user_trigger = "on";
                        sleep_case = 0;
                        process_now.is_wait = "off";
                    }
                    // other case
                    else
                    {
                        if (ready_queue.size() != 0)
                        {
                            kernel_mode = "schedule";
                            command = kernel_mode;

                            process_now = ready_queue.front();
                            ready_queue.pop_front();

                            process_now.state = "Ready";
                            process_now.schedule();

                            if (ready_queue.size() == 0 && waiting_queue.size() != 0)
                            {
                                wait_to_ready = "on";
                            }

                            user_trigger = "on";
                            sleep_case = 0;
                            process_now.is_wait = "off";
                        }
                        else
                        {
                            kernel_mode = "idle";
                            command = kernel_mode;
                        }
                    }
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

                    process_now.state = "Ready";

                    for (iter = process_list.begin(); iter != process_list.end(); iter++)
                    {
                        if (iter->parent_id == process_now.id)
                        {
                            if (iter->state == "dead" or iter->state == "Terminated")
                            {
                                process_now.state = "Waiting";
                                wait_trigger = "on";

                                break;
                            }
                        }
                    }
                    if (wait_trigger == "on")
                    {
                        string waiting_msg = "";
                        waiting_msg += process_now.id;
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

                        process_now.schedule();
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

                    process_now.state = "Terminated";
                    process_terminated = process_now;
                    int parent_id_now = process_now.parent_id;

                    if (process_now.id == 1)
                    {
                        end_trigger = "on";
                    }
                    else
                    {
                        // 337 help me!
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

                        process_now.schedule();
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
        answer += "2. command: ";
        answer += command;
        answer + "\n";

        // running
        string running = "none";

        if (process_now.state == "Running")
        {
            running = process_id;
            running += "(";
            running += process_now.name;
            running += ", ";
            running += process_now.parent_id;
            running += ")\n";
        }

        answer += "3. running: ";
        answer += running;
        answer += "\n";

        // ready
        string ready = "none";

        return 0;
    }