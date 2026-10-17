void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  int q1, q2, q3;
  c->proc = 0;

  for(;;){
    sti();
    acquire(&ptable.lock);

    q1 = 0;
    q2 = 0;
    q3 = 0;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE){
        if(p->currentq == 1)
          q1 = 1;
        else if(p->currentq == 2)
          q2 = 1;
        else if(p->currentq == 3)
          q3 = 1;
      }
    }

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE || p->currentq != 1)
        continue;

      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();
      c->proc = 0;
      p->runs++;

      if(p->runs >= 2){
        p->currentq = 2;
        p->runs = 0;
      }
    }

    if(!q1){
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE || p->currentq != 2)
          continue;

        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();
        c->proc = 0;
        p->runs++;

        if(p->runs >= 3){
          p->currentq = 3;
          p->runs = 0;
        }
      }
    }

    if(!q1 && !q2){
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE || p->currentq != 3)
          continue;

        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&(c->scheduler), p->context);
        switchkvm();
        c->proc = 0;
        p->runs++;
      }
    }

    release(&ptable.lock);
  }
}
