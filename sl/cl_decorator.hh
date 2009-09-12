#ifndef H_GUARD_CL_DECORATOR_H
#define H_GUARD_CL_DECORATOR_H

#include "cl_private.hh"

class ClDecoratorBase: public ICodeListener {
    public:
        virtual ~ClDecoratorBase() {
            delete slave_;
        }

        virtual void reg_type_db(
            cl_get_type_fnc_t       fnc,
            void                    *user_data)
        {
            slave_->reg_type_db(fnc, user_data);
        }

        virtual void file_open(
            const char              *file_name)
        {
            slave_->file_open(file_name);
        }

        virtual void file_close() {
            slave_->file_close();
        }

        virtual void fnc_open(
            const struct cl_location*loc,
            const char              *fnc_name,
            enum cl_scope_e         scope)
        {
            slave_->fnc_open(loc, fnc_name, scope);
        }

        virtual void fnc_arg_decl(
            int                     arg_id,
            const struct cl_operand *arg_src)
        {
            slave_->fnc_arg_decl(arg_id, arg_src);
        }

        virtual void fnc_close() {
            slave_->fnc_close();
        }

        virtual void bb_open(
            const char              *bb_name)
        {
            slave_->bb_open(bb_name);
        }

        virtual void insn(
            const struct cl_insn    *cli)
        {
            slave_->insn(cli);
        }

        virtual void insn_call_open(
            const struct cl_location*loc,
            const struct cl_operand *dst,
            const struct cl_operand *fnc)
        {
            slave_->insn_call_open(loc, dst, fnc);
        }

        virtual void insn_call_arg(
            int                     arg_id,
            const struct cl_operand *arg_src)
        {
            slave_->insn_call_arg(arg_id, arg_src);
        }

        virtual void insn_call_close() {
            slave_->insn_call_close();
        }

        virtual void insn_switch_open(
            const struct cl_location*loc,
            const struct cl_operand *src)
        {
            slave_->insn_switch_open(loc, src);
        }

        virtual void insn_switch_case(
            const struct cl_location*loc,
            const struct cl_operand *val_lo,
            const struct cl_operand *val_hi,
            const char              *label)
        {
            slave_->insn_switch_case(loc, val_lo, val_hi, label);
        }

        virtual void insn_switch_close() {
            slave_->insn_switch_close();
        }

    protected:
        ClDecoratorBase(ICodeListener *slave):
            slave_(slave)
        {
        }

    private:
        ICodeListener *slave_;
};

#endif /* H_GUARD_CL_DECORATOR_H */
