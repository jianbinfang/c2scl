//
// Created by ss on 2021/9/15.
//

//------------------------------------------------------------------------------
// C to hStreams:
//
// Peng Zhang(pengzhang_nudt@sina.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include "HPassManager.h"

//#define DEBUG_INFO
//#define GEN_CUDA
#define GEN_OCL

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.

// Override the method that gets called for each parsed top-level
// declaration.
bool MyASTConsumer::HandleTopLevelDecl(DeclGroupRef DR) {
    SourceLocation loc_e;
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        if (SM->isWrittenInMainFile((*b)->getLocation()))
            scopeGen->TraverseDecl(*b);
        loc_e=(*b)->getLocation();
    }
    int line_e=SM->getExpansionLineNumber(loc_e);

    //Test code.
    for (auto& s : CurScope->children) {
        if(s->pos->get_line()<line_e)
            continue;
        if (s->get_type() != 1)
            continue;
        HLoopInfo * root = (HLoopInfo *) s;
        std::deque<HLoopInfo *> loop_que;
        loop_que.push_back(root);
        while (loop_que.empty() == false) {
            HLoopInfo * loop = loop_que.front();
            loop_que.pop_front();
            for (auto &child : loop->children_loops) {
                /*
                if(child->type==2)
                {
                    HScopeIR * s_child_loop=s->children.front();
                    while(s_child_loop)
                    {
                        if (s_child_loop->pos->get_line()==child->pos->get_line())
                        {
                            child->decl_chain=s_child_loop->decl_chain;
                            child->access_chain=s_child_loop->access_chain;
                            break;
                        }
                        else
                        {
                            if(!s_child_loop->children.empty())
                                s_child_loop=s_child_loop->children.front();
                            else
                                break;
                        }
                    }
                }
                 */
                HScopeIR *child_scope=pos_to_scope(*child->pos);
                child->decl_chain=child_scope->decl_chain;
                child->access_chain=child_scope->access_chain;
                if (child->isParallelizable() == true) {
                    std::cout << "Loop at line " << child->get_start_line() << " can be parallel.\n";
                    //child->dump();
                    //Extract candidate kernel info from the parallelizable loops.
                    HLoopInfo * loop_c= static_cast<HLoopInfo *>(child);
                    kernelExtractor.nouse(loop_c);
//	  kernelExtractor.visitloop(loop_c);
                }
                else
                    loop_que.push_back(child);
            }
        }
    }

    return true;
}

struct HKernel_Info * MyFrontendAction::select_kernel () {
    struct HKernel_Info * ret = NULL;
    unsigned int max_insns = 0;
    //unsigned int max_overlap_mems = 0;
    //unsigned int max_mems = 0;
    for (auto &k_info : k_info_queue) {
        unsigned int overlap_mems = 0;
        unsigned int mems = 0;
        for (auto &mem_buf : k_info.mem_bufs) {
            //Pre transfer mem
            if (mem_buf.type & 1)
                mems++;
            if (mem_buf.type & 2)
                overlap_mems++;
            if (mem_buf.type & 4)
                overlap_mems++;
            if (mem_buf.type & 8)
                mems++;
        }
        mems += overlap_mems;

        if (k_info.insns >= max_insns) {
            max_insns = k_info.insns;
            ret = &k_info;
        }
    }
    return ret;
}

int MyFrontendAction::find_incall(CallExpr *call,Array_rw_chain * array_cur){
    int flag;
    for(unsigned i=0;i<call->getNumArgs();i++){
        Expr * args=call->getArg(i);
        if(isa<ArraySubscriptExpr>(args)){
            HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(args));
            if(av_infor->decl_var->get_name()==array_cur->name){
                flag=1;
                return flag;
            }
        }
    }
    return 0;
}
int MyFrontendAction::find_arr_name(BinaryOperator *rhs,Array_rw_chain * array_cur){
    int flag=0;
    if(isa<BinaryOperator>(rhs->getLHS()->IgnoreImpCasts()))
        flag=find_arr_name(dyn_cast<BinaryOperator>(rhs->getLHS()->IgnoreImpCasts()),array_cur);
    if(isa<ArraySubscriptExpr>(rhs->getLHS()->IgnoreImpCasts())){
        HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(rhs->getLHS()->IgnoreImpCasts()));
        if(av_infor->decl_var->get_name()==array_cur->name){
            flag=1;
            return flag;
        }
    }
    if(isa<BinaryOperator>(rhs->getRHS()->IgnoreImpCasts()))
        flag=find_arr_name(dyn_cast<BinaryOperator>(rhs->getRHS()->IgnoreImpCasts()),array_cur);
    if(isa<ArraySubscriptExpr>(rhs->getRHS()->IgnoreImpCasts())){
        HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(rhs->getRHS()->IgnoreImpCasts()));
        if(av_infor->decl_var->get_name()==array_cur->name){
            flag=1;
            return flag;
        }
    }
    return 0;
}
void MyFrontendAction::EndSourceFileAction() {
    //put the create buffer out the outermost loop
    int k_internel=0;
    HScopeIR * scope_cur;
    HPosition * pos_scope;
    pos_scope=new HPosition;
    std::vector<Array_rw_chain *> arr_rw_chain;
    std::vector<Array_rw_chain> arr_info_chain;
    for(auto k_info_can_array:k_info_queue){
        for(unsigned array_num=0;array_num<k_info_can_array.mem_bufs.size();array_num++){
            unsigned has_changed=0;
            unsigned arr_chain_id=0;
            hmem_xfer buffer=k_info_can_array.mem_bufs[array_num];
            if(arr_rw_chain.empty()){
                Array_rw_chain *arr=new Array_rw_chain;
                arr->start_place=k_info_can_array.enter_loop;
                arr->end_place=k_info_can_array.exit_loop;
                arr->name=buffer.buf_name;
                arr_rw_chain.push_back(arr);
                arr_chain_id=1;
            }
            for(auto array_cur:arr_rw_chain){
                if(array_cur->name==buffer.buf_name){
                    if(array_cur->end_place<k_info_can_array.exit_loop)
                    {
                        unsigned flag=0;
                        unsigned loop_id=0;
                        unsigned can_stay=1,must_d2h=0,must_h2d=0;
                        pos_scope->set_col(12);
                        pos_scope->set_line(array_cur->start_place);
                        scope_cur=pos_to_scope(*pos_scope);
                        scope_cur=scope_cur->parent;
                        for(auto child_scope:scope_cur->children){
                            if(child_scope->pos->get_line()==array_cur->start_place){
                                break;
                            }
                            loop_id++;
                        }
                        Stmt * compound_stmt=scope_cur->get_stmt();
                        if(isa<CompoundStmt>(compound_stmt)){
                            for (Stmt::child_iterator it = compound_stmt->child_begin(); it != compound_stmt->child_end(); ++it){
                                Stmt *it_i=*it;
                                if(flag==(loop_id+1)){
                                    if(isa<BinaryOperator>(it_i)){
                                        Expr *lhs = (dyn_cast<BinaryOperator>(it_i))->getLHS()->IgnoreImpCasts();
                                        Expr *rhs = (dyn_cast<BinaryOperator>(it_i))->getRHS()->IgnoreImpCasts();
                                        if(isa<ArraySubscriptExpr>(lhs)){
                                            HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(lhs));
                                            if(av_infor->decl_var->get_name()==array_cur->name){
                                                must_h2d=1;
                                                can_stay=0;
                                            }
                                        }
                                        if(isa<ArraySubscriptExpr>(rhs)){
                                            HAccess_Mem * av_infor = new HAccess_Mem(dyn_cast<ArraySubscriptExpr>(rhs));
                                            if(av_infor->decl_var->get_name()==array_cur->name){
                                                must_d2h=1;
                                                can_stay=0;
                                            }
                                        }
                                        if(isa<BinaryOperator>(rhs)){
                                            int has_buffer=0;
                                            has_buffer=find_arr_name(dyn_cast<BinaryOperator>(rhs),array_cur);
                                            if(has_buffer==1){
                                                must_d2h=1;
                                                can_stay=0;
                                            }
                                        }
                                    }
                                    if(isa<CallExpr>(it_i)){
                                        int has_buffer=0;
                                        has_buffer=find_incall(dyn_cast<CallExpr>(it_i),array_cur);
                                        if(has_buffer==1){
                                            can_stay=0;
                                            must_d2h=1;
                                            must_h2d=1;
                                        }
                                    }
                                    break;
                                }
                                if(isa<ForStmt>(it_i)){
                                    flag++;
                                    continue;
                                }
                            }
                        }
                        if(can_stay==1){
                            array_cur->end_place=k_info_can_array.exit_loop;
                            has_changed=1;
                        }
                    }
                }

                if(has_changed==1){
                    for(unsigned k_id=0;k_id<k_info_queue.size();k_id++){
                        if(k_info_queue[k_id].enter_loop==array_cur->start_place)
                            for(auto buffer_changed:k_info_queue[k_id].mem_bufs)
                                if(buffer_changed.buf_name==array_cur->name){
                                    k_info_queue[k_id].enter_loop2=array_cur->end_place;
                                    has_changed=0;
                                    break;
                                }
                        if(has_changed==0)
                            break;
                    }
                }
            }
            if(arr_chain_id==0)
            {
                Array_rw_chain *new_array=new Array_rw_chain;
                new_array->name=buffer.buf_name;
                new_array->start_place=k_info_can_array.enter_loop;
                new_array->end_place=k_info_can_array.exit_loop;
                arr_rw_chain.push_back(new_array);
            }
        }
    }
    if(!arr_rw_chain.empty())
        arr_info_chain.push_back(*arr_rw_chain[0]);
    for(int i=0;i<arr_rw_chain.size();i++){
        unsigned arr_id=0;
        for(int j=0;j<arr_info_chain.size();j++){
            arr_id++;
            if(arr_rw_chain[i]->name==arr_info_chain[j].name){
                if(arr_rw_chain[i]->start_place==arr_info_chain[j].start_place){
                    if(arr_rw_chain[i]->end_place>arr_info_chain[j].end_place)
                        arr_info_chain[j].end_place=arr_rw_chain[i]->end_place;
                    arr_id=0;
                }
                if(arr_rw_chain[i]->end_place==arr_info_chain[j].end_place)
                {
                    if(arr_rw_chain[i]->start_place<arr_info_chain[j].start_place)
                        arr_info_chain[j].start_place=arr_rw_chain[i]->start_place;
                    arr_id=0;
                }
            }

        }
        if(arr_id==arr_info_chain.size())
            arr_info_chain.push_back(*arr_rw_chain[i]);
    }

    for(auto k_info_can:k_info_queue)
        for(auto k_info_cant:k_info_queue_cant){
            if(k_info_can.enter_loop>k_info_cant.enter_loop&&k_info_can.exit_loop<k_info_cant.exit_loop)
            {
                k_info_queue[k_internel].create_mem_site=k_info_cant.create_mem_site;
                k_internel++;
            }
        }
    k_internel=0;
    for(auto k_info:k_info_queue){
        generator.set_enter_loop(k_info.enter_loop);
        generator.set_exit_loop(k_info.exit_loop);
        generator.set_init_site(k_info.init_site);
        generator.set_init_site_e(k_info.init_site_e);
        generator.set_create_mem_site (k_info.create_mem_site);
        generator.set_finish_site (k_info.finish_site);
        generator.set_include_site (k_info.last_include);
        generator.set_cuda_site (k_info.init_site);
        generator.set_enter_loop2(k_info.enter_loop2);
        generator.set_length_var(k_info.length_var);
        generator.set_loop_var(k_info.loop_var);
        generator.set_start_index(k_info.start_index);
        generator.set_replace_line(k_info.replace_line);
        struct hvar_decl var;
        unsigned args=0,points=0,vars=0;
        generator.push_back_args(args);
        generator.push_back_pointernum(points);
        generator.push_back_valnum(vars);
        for (unsigned i = 0; i < k_info.val_parms.size(); i++)
        {
            generator.add_kernel_arg(k_info.val_parms[i],k_internel);
        }
        for (unsigned i = 0; i < k_info.pointer_parms.size(); i++)
        {
            generator.add_kernel_arg(k_info.pointer_parms[i],k_internel);
        }
        for (unsigned i = 0; i < k_info.local_parms.size(); i++)
        {
            generator.add_local_var(k_info.local_parms[i],k_internel);
        }
        unsigned dim=0;
        for (unsigned i = 0; i < k_info.mem_bufs.size(); i++)
        {
            generator.add_mem_xfer(k_info.mem_bufs[i],k_internel);
            if(i==0)
                dim=k_info.mem_bufs[i].dim;
            if(dim>k_info.mem_bufs[i].dim)
                dim=k_info.mem_bufs[i].dim;
        }
        generator.set_kernel_dim(dim);
        k_internel++;
    }
    for(int i=0;i<arr_info_chain.size();i++)
        generator.set_arr_chain(arr_info_chain[i]);
    generator.set_logical_streams (2);
    generator.set_task_blocks (4);
    generator.generateOCLDevFile();
    generator.generateOCLHostFile();
    generator.generateOCLHeadFile();
    return;

/*
    struct HKernel_Info * k_info = select_kernel ();
    if (k_info == NULL)
        return;

    generator.set_enter_loop(k_info->enter_loop);
    generator.set_exit_loop(k_info->exit_loop);
    generator.set_init_site(k_info->init_site);
    generator.set_init_site_e(k_info->init_site_e);
    generator.set_create_mem_site (k_info->create_mem_site);
    generator.set_finish_site (k_info->finish_site);
    generator.set_include_site (k_info->last_include);
    generator.set_cuda_site (k_info->init_site);

    struct hvar_decl var;
#ifndef GEN_OCL
    var.type_name = "int";
    var.var_name = "start_index";
    var.type = 2;
    generator.add_kernel_arg(var);

    var.var_name = "end_index";
    generator.add_kernel_arg(var);
#endif

    generator.set_length_var(k_info->length_var);
    generator.set_loop_var(k_info->loop_var);
    generator.set_start_index(k_info->start_index);
    generator.set_replace_line(k_info->replace_line);

    for (unsigned i = 0; i < k_info->val_parms.size(); i++)
    {
        generator.add_kernel_arg(k_info->val_parms[i]);
    }
    for (unsigned i = 0; i < k_info->pointer_parms.size(); i++)
    {
        generator.add_kernel_arg(k_info->pointer_parms[i]);
    }
    for (unsigned i = 0; i < k_info->local_parms.size(); i++)
    {
        generator.add_local_var(k_info->local_parms[i]);
    }
    for (unsigned i = 0; i < k_info->mem_bufs.size(); i++)
    {
        generator.add_mem_xfer(k_info->mem_bufs[i]);
    }

    generator.set_logical_streams (2);
    generator.set_task_blocks (4);

    //Generate OpenCL Code
#ifdef GEN_OCL
    generator.generateOCLDevFile();
    generator.generateOCLHostFile();
    return;
#endif

    //Generate CUDA Code
#ifdef GEN_CUDA
    generator.generateCUDAFile();

    return;
#endif

    //Generate hStreams Code by default
    generator.generateDevFile();
    generator.generateHostFile();
    return;
    */
}

std::unique_ptr<ASTConsumer> MyFrontendAction::CreateASTConsumer(CompilerInstance &CI,
                                                                 StringRef file) {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    std::string f_name = file.str();
    int f_index = f_name.find_last_of('/');
    f_name = f_name.substr(f_index+1,-1);
    generator.set_InputFile (f_name);

    return std::unique_ptr<MyASTConsumer>(new MyASTConsumer(k_info_queue,k_info_queue_cant));
}

