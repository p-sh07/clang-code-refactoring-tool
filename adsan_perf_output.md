//=======================================================================//
1.AdSan with clang on leak_example.cpp:
//----- Before Refactor ---------------------------------------------------------------------------------------------------------------------------------------------------//
   ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo clang++ -g -fsanitize=address ../tests/tests_data/leak_example.cpp -o leak_before_refactor
   ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ ./leak_before_refactor
   =================================================================
   ==17240==ERROR: AddressSanitizer: new-delete-type-mismatch on 0xe6e56bbe0010 in thread T0:
   object passed to delete has wrong type:
   size of the allocated type:   8 bytes;
   size of the deallocated type: 1 bytes.
   #0 0xb60992e1cf40 in operator delete(void*, unsigned long) (/home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/leak_before_refactor+0x11cf40) (BuildId: bee7032633c08f1d1cd39d8a83a2895f20951c4d)
   #1 0xb60992e1d774 in main /home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/../tests/tests_data/leak_example.cpp:18:5
   #2 0xeac56c9522d8 in __libc_start_call_main csu/../sysdeps/nptl/libc_start_call_main.h:58:16
   #3 0xeac56c9523b8 in __libc_start_main csu/../csu/libc-start.c:360:3
   #4 0xb60992d355ec in _start (/home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/leak_before_refactor+0x355ec) (BuildId: bee7032633c08f1d1cd39d8a83a2895f20951c4d)

0xe6e56bbe0010 is located 0 bytes inside of 8-byte region [0xe6e56bbe0010,0xe6e56bbe0018)
allocated by thread T0 here:
#0 0xb60992e1c290 in operator new(unsigned long) (/home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/leak_before_refactor+0x11c290) (BuildId: bee7032633c08f1d1cd39d8a83a2895f20951c4d)
#1 0xb60992e1d73c in main /home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/../tests/tests_data/leak_example.cpp:17:17
#2 0xeac56c9522d8 in __libc_start_call_main csu/../sysdeps/nptl/libc_start_call_main.h:58:16
#3 0xeac56c9523b8 in __libc_start_main csu/../csu/libc-start.c:360:3
#4 0xb60992d355ec in _start (/home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/leak_before_refactor+0x355ec) (BuildId: bee7032633c08f1d1cd39d8a83a2895f20951c4d)

SUMMARY: AddressSanitizer: new-delete-type-mismatch (/home/ps/dev/clang-code-refactoring-tool/cmake-build-debug/leak_before_refactor+0x11cf40) (BuildId: bee7032633c08f1d1cd39d8a83a2895f20951c4d) in operator delete(void*, unsigned long)
==17240==HINT: if you don't care about these errors you may set ASAN_OPTIONS=new_delete_type_mismatch=0
==17240==ABORTING

//------After Refactor --------------------------------------------------------------------------------------------------------------------------------------------------------//
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo clang++ -g -fsanitize=address ../tests/tests_data/leak_example.cpp -o leak_after_refactor
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ ./leak_after_refactor
Program finished successfully

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

//=======================================================================//
2.perf with clang on perf_example.cpp: *running on ubuntu VM so results are inconclusive

//----- Before Refactor -------------------------------------------------//
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo clang++ -g -O2 ../tests/tests_data/perf_example.cpp -o perf_before_refactor
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo perf record -F 100 -g ./perf_before_refactor
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.051 MB perf.data (178 samples) ]
Killed
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo perf report

100.00%     0.00%  perf_before_ref  perf_before_refactor  [.] _start                     ◆
+  100.00%     0.00%  perf_before_ref  libc.so.6             [.] __libc_start_main@@GLIBC_2.▒
+  100.00%     0.00%  perf_before_ref  libc.so.6             [.] __libc_start_call_main     ▒
+  100.00%     5.62%  perf_before_ref  perf_before_refactor  [.] main                       ▒
+   92.70%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] el0t_64_sync               ▒
+   92.70%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] el0t_64_sync_handler       ▒
+   92.13%     1.69%  perf_before_ref  libc.so.6             [.] __memset_zva64             ▒
+   88.20%     7.30%  perf_before_ref  [kernel.kallsyms]     [k] el0_da                     ▒
+   85.39%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] do_mem_abort               ▒
+   85.39%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] do_translation_fault       ▒
+   84.27%     1.12%  perf_before_ref  [kernel.kallsyms]     [k] do_page_fault              ▒
+   81.46%     0.56%  perf_before_ref  [kernel.kallsyms]     [k] handle_mm_fault            ▒
+   80.90%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] __handle_mm_fault          ▒
+   80.34%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] handle_pte_fault           ▒
+   75.84%     0.56%  perf_before_ref  [kernel.kallsyms]     [k] do_anonymous_page          ▒
+   71.35%     0.56%  perf_before_ref  [kernel.kallsyms]     [k] alloc_pages_mpol           ▒
+   70.22%     0.56%  perf_before_ref  [kernel.kallsyms]     [k] __alloc_frozen_pages_noprof▒
+   69.10%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] alloc_anon_folio           ▒
+   66.85%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] vma_alloc_zeroed_movable_fo▒
+   66.85%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] vma_alloc_folio_noprof     ▒
+   66.85%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] folio_alloc_mpol_noprof    ▒
+   52.81%     1.12%  perf_before_ref  [kernel.kallsyms]     [k] get_page_from_freelist     ▒
+   48.31%    48.31%  perf_before_ref  [kernel.kallsyms]     [k] __pi_clear_page            ▒
+   19.10%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] __alloc_pages_slowpath.cons▒
+   16.29%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] try_to_free_pages          ▒
+   16.29%     0.00%  perf_before_ref  [kernel.kallsyms]     [k] do_try_to_free_pages 


//----- After Refactor -------------------------------------------------//
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo clang++ -g -O2 ../tests/tests_data/perf_example.cpp -o perf_after_ref
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo perf record -F 100 -g ./perf_after_ref
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.072 MB perf.data (340 samples) ]
ps@ps-pc:~/dev/clang-code-refactoring-tool/cmake-build-debug$ sudo perf report

+  100.00%     0.00%  perf_after_ref  perf_after_ref     [.] _start
+  100.00%     0.00%  perf_after_ref  libc.so.6          [.] __libc_start_main@@GLIBC_2.34
+  100.00%     0.00%  perf_after_ref  libc.so.6          [.] __libc_start_call_main
+  100.00%    38.24%  perf_after_ref  perf_after_ref     [.] main
+   58.82%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] el0t_64_sync
+   58.82%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] el0t_64_sync_handler
+   52.94%     2.06%  perf_after_ref  libc.so.6          [.] __memset_zva64
+   50.88%     4.71%  perf_after_ref  [kernel.kallsyms]  [k] el0_da
+   46.18%     0.29%  perf_after_ref  [kernel.kallsyms]  [k] do_mem_abort
+   45.88%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] do_translation_fault
+   45.88%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] do_page_fault
+   45.29%     0.88%  perf_after_ref  [kernel.kallsyms]  [k] handle_mm_fault
+   44.12%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] __handle_mm_fault
+   44.12%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] handle_pte_fault
+   43.82%     0.29%  perf_after_ref  [kernel.kallsyms]  [k] do_anonymous_page
+   40.29%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] alloc_anon_folio
+   38.82%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] vma_alloc_zeroed_movable_folio
+   38.82%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] vma_alloc_folio_noprof
+   38.82%     0.29%  perf_after_ref  [kernel.kallsyms]  [k] alloc_pages_mpol
+   38.53%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] folio_alloc_mpol_noprof
+   38.53%     0.00%  perf_after_ref  [kernel.kallsyms]  [k] __alloc_frozen_pages_noprof
+   35.59%     0.29%  perf_after_ref  [kernel.kallsyms]  [k] get_page_from_freelist
+   33.24%    33.24%  perf_after_ref  [kernel.kallsyms]  [k] __pi_clear_page
+    7.94%     0.00%  perf_after_ref  libc.so.6          [.] cfree@GLIBC_2.17
+    7.94%     0.00%  perf_after_ref  libc.so.6          [.] __munmap
+    7.94%     0.29%  perf_after_ref  [kernel.kallsyms]  [k] zap_pte_range

//----- Analysis -------------------------------------------------//

Before refactor: Kernel page faults/memory management dominated:
        el0_da (data abort): 88.20%
        do_mem_abort: 85.39%
        handle_mm_fault: 81.46%
        Memory allocation (alloc_pages_mpol, get_page_from_freelist): ~70% total

After refactor: These dropped dramatically:

        el0_da: 50.88% (↓43% relative)
        do_mem_abort: 46.18% (↓46% relative)
        handle_mm_fault: 45.29% (↓44% relative)

Time spent in main: 5.62% -> 38.24% => more useful work per cycle

