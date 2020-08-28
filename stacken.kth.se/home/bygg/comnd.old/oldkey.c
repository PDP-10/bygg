
static int old_keyword(void)
{
  static unsigned char fyfan[] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xfb, 0x00, 0x3f,
    0x80, 0x00, 0x00, 0x1f,
    0x80, 0x00, 0x00, 0x1f,
  };
  static breakset keybrk = { fyfan, 128, NULL, 0 };

  int matchcount;
  char* matchptr;
  int matchrest;

  char c;
  int i;
  keytab* kt;
  keyword* kw;
  
  kt = (keytab*) this->data;
  if (this->brk != NULL) {
    /* should handle this */
  }

  skipblanks();
  for (;;) {
    c = nextch();
    if (c == '\t') {
      /* try to complete */
      uninput();
      cmdip();
      matchcount = 0;
      for (i = 0; i < kt->count; i += 1) {
	kw = &kt->keys[i];
	if (kw->flags & KEY_NOR) {
	  continue;
	}
	if (atommatch(kw->key)) {
	  if (kw->key[atomptr] == (char) 0) {
	    matchcount = 1;
	    matchrest = 0;
	    break;
	  }
	  if (matchcount == 0) {
	    matchptr = &kw->key[atomptr];
	    matchrest = strlen(matchptr);
	  } else {
	    char* p = matchptr;
	    char* q = &kw->key[atomptr];
	    matchrest = 0;

	    while ((*p != (char) 0) && (*p++ == *q++)) {
	      matchrest += 1;
	    }
	  }
	  matchcount += 1;
	}
      }
      if (matchcount == 0) {
	/*
	** No match on current FDB.  We should check the rest of the
	** FDB chain for possible completion.  This might get messy...
	*/
	beep();
      }
      if (matchcount == 1) {
	for (i = 0; i < matchrest; i += 1) {
	  cmdib(matchptr[i]);
	  atomstore(matchptr[i]);
	}
	cmdib(' ');
	atomdone();
	break;
      }
      if (matchcount > 1) {
	if (matchrest == 0) {
	  beep();
	} else {
	  for (i = 0; i < matchrest; i += 1) {
	    cmdib(matchptr[i]);
	    atomstore(matchptr[i]);
	  }
	}
      }
      continue;
    }
    if (c == '?') {
      int maxlen;		/* length of longest keyword. */
      int maxwpl;		/* max number of kw's per line. */
      int wpl;			/* kw's on this line. */
      int len;

      if (this->flags & CM_SDH) {
	if (this->help != NULL) {
	  dohelp(this->help);
	}
	longjmp(cm_pfbuf, 1);
      }

      starthelp("keyword, ");

      maxlen = 0;
      for (i = 0; i < kt->count; i += 1) {
	kw = &kt->keys[i];
	if (atommatch(kw->key)) {
	  if (kw->flags & KEY_INV) {
	    continue;
	  }
	  len = strlen(kw->key);
	  if (len > maxlen) maxlen = len;
	}
      }

      maxwpl = (79 + 2) / (maxlen + 3);
      wpl = 0;

      matchcount = 0;
      for (i = 0; i < kt->count; i += 1) {
	kw = &kt->keys[i];
	if (atommatch(kw->key)) {
	  if (kw->flags & KEY_INV) {
	    continue;
	  }
	  if (matchcount == 0) {
	    printf("one of the following:");
	  }
	  if (kt->flags & KT_MWL) {
	    if (wpl == 0) {
	      printf("\n ");
	    }
	    printf("%s", kw->key);
	    wpl += 1;
	    if (wpl == maxwpl) {
	      wpl = 0;
	    } else {
	      spaces(3 + maxlen - strlen(kw->key));
	    }
	  } else {
	    printf("\n%s", kw->key);
	    if (kw->descr != NULL) {
	      spaces(2 + maxlen - strlen(kw->key));
	      printf("- %s", kw->descr);
	    }
	  }
	  matchcount += 1;
	}
      }
      if (matchcount == 0) {
	printf("(no keyword matches current input)");
      }
      longjmp(cm_pfbuf, 1);	/* Go back and check the other alternatives. */
    }
    if (breakchar(c, &keybrk)) {
      cmdip();
      break;
    }
    atomstore(c);
  }
  atomdone();

  matchcount = 0;
  if (atomptr > 0) {
    for (i = 0; i < kt->count; i += 1) {
      kw = &kt->keys[i];
      if (!(kw->flags & KEY_NOR)) {
	if (atommatch(kw->key)) {
	  pval.kw = kw;
	  if (kw->key[atomptr] == (char) 0) {
	    return;
	  }
	  matchcount += 1;
	}
      }
    }
  }
  if (matchcount == 0) {
    noparse("no such keyword");
  }
  if (matchcount > 1) {
    noparse("ambigous keyword");
  }
}
