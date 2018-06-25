import poplib
pop = poplib.POP3("localhost", 5555)
pop.set_debuglevel(1)
pop.user("a")
pop.pass_("a")
print(pop.stat(), pop.list(), pop.retr(1))
pop.close()
