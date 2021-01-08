from work_queue import *
import sys
import time

def compose_task(rate, block):
	outfile = "results%f_%d.csv" % (rate, block)
	submission_outfile = "dist_sweep_results/" + outfile
	command = "bash script.sh results%f_%d.csv %f %d" % (rate, block, rate, block)

	t = Task(command)
    
	t.specify_file("/usr/bin/bash", "bash", WORK_QUEUE_INPUT, cache=True)
	t.specify_file("env.tar.gz", "env.tar.gz", WORK_QUEUE_INPUT, cache=True)
	t.specify_file("datasets/cifar-10-batches-py", "datasets/cifar-10-batches-py", WORK_QUEUE_INPUT, cache=True)
	t.specify_file("resnet.py", "resnet.py", WORK_QUEUE_INPUT, cache=True)
	t.specify_file("script.sh", "script.sh", WORK_QUEUE_INPUT, cache=True)
	t.specify_file(submission_outfile, outfile, WORK_QUEUE_OUTPUT, cache=False)
	t.specify_cores(1)
	t.specify_memory(4000)
	t.specify_disk(4000)
	return t



def main():
	start = time.time()
	try:
		PORT = 9123
		debug_log = "dist_sweep_debug.log"
		resources_monitor_dir = "dist_sweep_resources_summary"
		transactions_log = "dist_sweep_transactions.log"
		project_name = "dist_sweep"
		q = WorkQueue(port = PORT, debug_log = debug_log)
		q.enable_monitoring(resources_monitor_dir)
		q.specify_transactions_log(transactions_log)
		q.specify_name(project_name)
	except:
		print("Instantiation of Work Queue failed.")
		sys.exit(1)

	print("Listening on port %d..." % q.port)

	drop_rate_range = [i/100 for i in range(5, 100, 5)]
	res_block_range = [i for i in range(1, 21)]
	for rate in drop_rate_range:
		for block in res_block_range:
			t = compose_task(rate, block)
			taskid = q.submit(t)
			print("Submitted task (id# %d): %s" % (taskid, t.command))
			with open("hyper_par_to_task.txt", "a") as f:
				f.write("Task id :" + str(taskid)+ " - rate, block: "+ str(rate)+", "+str(block)+"\n")
	print("waiting for tasks to complete...")
	whitelist = []
	blacklist = []
	while not q.empty():
		t = q.wait(5)
		if t:
			print("task (id# %d) complete: %s (return code %d)" % (t.id, t.command, t.return_status))
			if t.return_status == 0:
				if t.hostname not in whitelist:
					whitelist.append(t.hostname)
			if t.return_status != 0:
				print("stdout:\n{}".format(t.output))
				print("Blacklisting host: %s" % t.hostname)
				q.blacklist(t.hostname)
				blacklist.append(t.hostname)
				q.submit(t)
				print("Resubmitted task (id# %s): %s" % (t.id, t.command))

	print("All tasks complete.")
	print("Whitelist:", whitelist)
	print("Blacklist:", blacklist)
	now = time.time()-start
	print("Total workflow of dist_sweep takes " + str(now) + " seconds.")
	sys.exit(0)

if __name__ == '__main__':
	main()
