/*
	WELCOME : )
	YOU MUST DO NOT REMOVE EXISTING CODE
	KEEP THE CODE AND ADD YOUR CODE TO THE SECTION MARKED AS DIY
	DO NOT REMOVE PRINT STATEMENT (e.g, std::cout ...)

	Jonghyeok Park
	jonghyeok_park@korea.ac.kr
*/

#include "system.h"
#include "lock.h"

lock::lock() {}

#define LOCK_NAME(lockType) \
    ((lockType) == LOCK_TYPE::SHARED ? "S-LOCK" : \
    (lockType) == LOCK_TYPE::EXCLUSIVE ? "X-LOCK" : "UNKNOWN")

#define OP_NAME(op) \
    ((op) == OP::READ ? "READ" : \
    (op) ==  OP::WRITE ? "WRITE" : "UNKNOWN")


void lock::print_lock_list() {
    for (auto iter = lock_list.begin(); iter != lock_list.end(); ) {
        auto obj = iter->first;
        auto lock_info = iter->second;
        std::cout << "--- OBJECT: " << obj.name << "---\n";
        for (auto info : lock_info) {
            std::cout << LOCK_NAME(info.second) << " acquired by TX" << info.first.id << "\n";
        }
        std::cout << "--------------------\n";

        ++iter;
    }
}

/*
	Acquire lock to the given object with lock type following these steps.

    step1. Find the existing locks on the object and traverse all locks
    step2. Check whether the transaction can acuire the requested lock.
		-- If the object is already locked by a younger transaction and there is a conflict, we will abort (rollback) the younger transaction.
    step3. If the transaction can acquire lock, it adds lock list
    step4. If the transaction can not acuqire lock then we need to call rollback function.

	[HINT] 
	- You need to see the `lock_list` data structure at `system.h` file.
	```
	// case1. find the all locks in lock_list for given object (e.g., obj). 
		
		auto it = lock_list.find(obj);
		if (it != lock_list.end()) {
			// it means we found something ... 
		}

	// case2. insert data into lock_list upon transaction cna acquire the lock

		lock_list[obj].push_back(std::pair<trx_t, LOCK_TYPE>(trx,type));
	
	```
	- Lock list does not allow duplicate lock. For example, 
	- We only conisder two types of locks: LOCK_TYPE::SHARED and LOCK_TYPE::EXCLUSIVE
*/
bool lock::acquire_lock(trx_t trx, object_t obj, LOCK_TYPE type) {	
	// DIY

	auto it = lock_list.find(obj);
//bool first_lock_on_obj = (it == lock_list.end());
	if (it != lock_list.end()) {
		auto &vec = it->second;

		for (auto vit = vec.begin(); vit != vec.end(); ) {
			trx_t holder      = vit->first;
			LOCK_TYPE h_type  = vit->second;

			if (holder.id == trx.id) {
				if (h_type == LOCK_TYPE::SHARED && type == LOCK_TYPE::EXCLUSIVE) {

					std::vector<trx_t> younger_list;
					bool older_exists = false;

					for (auto &p : vec) {
						if (p.first.id == trx.id) continue;

						if (trx.timestamp < p.first.timestamp) {
							younger_list.push_back(p.first);
						} else {
							older_exists = true;
						}
					}

					if (older_exists) {
						return false;
					}
					for (auto &ytrx : younger_list) {
						rollback(ytrx);
					}
auto self_it = std::find_if(vec.begin(), vec.end(), [&](auto &p){ return p.first.id == trx.id; });
if (self_it != vec.end()) {
	vec.erase(self_it);
}
lock_list[obj].push_back(std::make_pair(trx, LOCK_TYPE::EXCLUSIVE));
					return true;
				}
			}

			bool conflict = (type == LOCK_TYPE::EXCLUSIVE) || (h_type == LOCK_TYPE::EXCLUSIVE);
			if (!conflict) { ++vit; continue; }

			if (trx.timestamp < holder.timestamp) {
				rollback(holder);
				it = lock_list.find(obj);
				if (it == lock_list.end()) break;
				vec = it->second;
				vit = vec.begin();
				continue;
			} else {
				return false;
			}
		}
	}

	lock_list[obj].push_back(std::make_pair(trx, type));

	return true;
}


/*
	Traverse the lock_list and remove all locks acquired by given transaction.
*/

void lock::release_lock(trx_t trx) {	
	std::cout << "[LOCK RELEASE] trx" << trx.id << "\n";
	std::cout << "Before releasing locks (lock list status): \n";
	print_lock_list();
	
	// DIY

    for (auto it = lock_list.begin(); it != lock_list.end(); ) {
        auto &vec = it->second;

        vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const auto &p){ return p.first.id == trx.id; }), vec.end());

        if (vec.empty())
            it = lock_list.erase(it);
        else
            ++it;
    }

	
	std::cout << "After releasing locks (lock list status): \n";
	print_lock_list();
}

/*
	Rollback the transaction.
	step1. Release all locks held by the transaction.
	step2. Remove the actions performed by the transaction from the `output` vector.
	setp3. Remove the remaining actions of the transaction being rolled back from the `actions` vector
			, and then append the actions of the rollbacked transaction to the end of the actions vector. 
			For example, if the actions of T1 (i.e., rollbacked trx) are R1(A), R1(B), and W1(C), then R1(A), R1(B), and W1(C) should be re-executed.
	step4. Reset the timestamp (YOU DO NOT NEED TO MODIFY) 

	[HINT] check `actions` vector, we record transaction's actions in the `trx.actions` vector.
*/

void lock::rollback(trx_t trx) {	
	std::cout << "[ROLLBACK] TX" << trx.id << " is rollbeck!\n";
	// DIY	
    release_lock(trx);

	std::string prev_out = "";
	for (auto it = output.begin(); it != output.end(); ) {
		if (!it->empty() && (*it)[1] - '0' == trx.id) {
			std::cout << prev_out << " " << *it << " erase " << *it << "\n";
			it = output.erase(it);
			continue;
		}
		prev_out = *it;
		++it;
	}

    std::vector<std::string> temp;
    for (auto it = actions.begin(); it != actions.end(); ) {
        if (!it->empty() && (*it)[1] - '0' == trx.id) {
            temp.push_back(*it);
            it = actions.erase(it);
        } else {
            ++it;
        }
    }

	actions.insert(actions.end(), trx.actions.begin(), trx.actions.end());

	// DO NOT MODIFY
	trx.timestamp = global_counter++;
}


/*
	Process the given action. 
	For example, if the action is R1(A), then execute(trx 1, object A, Read) is called.
	A read operation must acquire LOCK_TYPE::SHARED, while a write operation must acquire LOCK_TYPE::EXCLUSIVE. 
	Neither operation performs the actual read or write; they only acquire the lock. 
	In this case, if the lock is successfully obtained, it returns STATUS::SUCCESS; if not, it returns STATUS::BLOCKED.
	If it returns STATUS::SUCCESS You have to add action into `output` vector (see COMMIT case).
	Upon commit, all locks held by the transaction are released, and the action is added to the output vector. Then, STATUS::COMMIT is returned.
*/
 
STATUS lock::execute(std::string action, trx_t trx, object_t obj) {
	char opcode = action[0];
	OP op;
	switch (opcode){
		case 'R': op = OP::READ; break;
		case 'W': op = OP::WRITE; break;
		case 'C': op = OP::COMMIT; break;
		default: op = OP::NONE;  break;
	}; 

	if (op == OP::READ || op == OP::WRITE) {
		std::cout << "[" << OP_NAME(op) << "]" << " TRX" << trx.id << " OBJECT: " << obj.name << "\n";
		LOCK_TYPE type = (op == OP::READ) ? LOCK_TYPE::SHARED : LOCK_TYPE::EXCLUSIVE;	
		// DIY

		bool ok = acquire_lock(trx, obj, type);

		if (ok) {
			std::cout << action << " success\n";
			output.push_back(action);
			return STATUS::SUCCESS;

		} else {
			std::cout << action << " is blocked\n";
			return STATUS::BLOCKED;
		}
		
	} else if (op == OP::COMMIT) {
		// DIY

		release_lock(trx);

		std::cout << "[COMMIT] TRX" << trx.id << "\n";
		output.push_back(action);
		return STATUS::COMMIT;
	} else {
		std::cout << "WRONG OPERATION\n";
	}

	return STATUS::NONE;
}

/*
	Perform the actions sequentially. 
	The actions vector is parsed from the transaction schedule. 
	
	[HINT] 
	1. Call the execute() function to perform the actions sequentially. 
	2. The result of the most recently executed action should be stored in the `ret` variable.
	3. The waiting_queue is globally accessible. 
*/
void lock::run() {

	STATUS ret = STATUS::NONE;
	for (auto it = actions.begin(); it != actions.end(); ) {
		if (actions.size() == 0) break;
	
		// Now we can parse action with transaction id (`tid`) and object id (`oid`) 
		std::string op = *it;
		int tid = (int)(op[1] - '0');
		char oid = op[3];
	
		// DIY

		// step1. If the result of the most recently executed action is a transaction commit, 
		// first iterate through the waiting_queue and execute the actions (i.e., call the execute() function).
		// If the return value is STATUS::BLOCKED, put it back into the waiting_queue.
 
		// TODO DIY
		if (ret == STATUS::COMMIT) {
			std::vector<std::string> tmp(waiting_queue);
			waiting_queue.clear();

			for (auto &wop : tmp) {
				int wtid = (int)(wop[1] - '0');
				char woid = wop[0] == 'C' ? '\0' : wop[3];

				STATUS wret = execute(wop, trx_map[wtid], obj_map[woid]);
				if (wret == STATUS::BLOCKED)
					waiting_queue.push_back(wop);
			}
		} 
	

		// step2. For a read/write transaction, check if the transaction is already present in the waiting queue.
		// If the transaction exists in the waiting_queue, set `blocked` variable to false. This indicates that current transaction can not proceed. 
		// Refer to step4, we can re-run the remainging actions in the waiting_list.
		// TODO DIY
		bool blocked = false;
		for (auto &q : waiting_queue) {
			if ((q[1]-'0') == tid) {
//std::cout << q << " is blocked!\n";
				waiting_queue.push_back(op);
				blocked = true;
				break;
			}
		}
/*
		bool already_waiting = std::any_of(waiting_queue.begin(), waiting_queue.end(), [&](const std::string &w){ return (int)(w[1] - '0') == tid; });

		if (already_waiting) {
			auto first_waiting =
				std::find_if(waiting_queue.begin(), waiting_queue.end(),
							[&](const std::string& w){
								return (int)(w[1] - '0') == tid;
							});

			//if (first_waiting != waiting_queue.end())
				//std::cout << *first_waiting << " is blocked!\n";
			//else
				//std::cout << op << " is blocked!\n";

			waiting_queue.push_back(op);

			blocked = true;
		}
*/

		// step3. If blocked is false, call the execute() function for the given action. If the transaction is blocked, insert it into the waiting_list for future processing.
		// Here, YOU DO NOT HAVE TO MODIFY THE CODE. 
		if (!blocked) {
			ret = execute(op, trx_map[tid], obj_map[oid]);
			if (ret == STATUS::BLOCKED) {
				waiting_queue.push_back(op);
			}
 
			print_lock_list();
		}

		// DO NOT MODIFY
		it = actions.erase(it);	
	}

	/* DO NOT MODIFY */
	/* =========================================================================== */	
	// step4. if waiting queue is not empty; we need to re-run
	if (waiting_queue.size() !=0) {
		actions.clear();
		actions.insert(actions.end(), waiting_queue.begin(), waiting_queue.end());
		waiting_queue.clear();
		run();

	} else {
		// print final output
		std::cout << "====== final state ======\n";
		for (auto &o : output) {
			std::cout << o << " ";
		}
		std::cout << "\n";	
	}
	/* =========================================================================== */	

}
