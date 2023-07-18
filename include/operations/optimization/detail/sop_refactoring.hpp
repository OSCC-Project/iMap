// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Shanghai Anlogic Infotech Co.,Ltd.
// Copyright (c) 2023-2025 Peking University
//
// iMAP-FPGA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************

#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/isop.hpp>
#include <kitty/operators.hpp>
#include <sstream>
#include <unordered_map>
#include <vector>

iFPGA_NAMESPACE_HEADER_START

    namespace detail
{
    /**
     * @brief wether this cube contain the lit
     */
    inline bool cube_has_lit(uint64_t const cube, uint64_t const lit)
    {
        return (cube & (static_cast<uint64_t>(1) << lit)) > 0;
    }

    /**
     * @brief count the num of lit in a cube
     */
    inline uint32_t cube_count_lit(uint64_t cube)
    {
        uint32_t count = 0;
        while (cube)
        {
            cube &= cube - 1u;
            ++count;
        }
        return count;
    }

    /**
     * @brief find the first literal which appears more than once
     */
    inline int64_t sop_lit_occur(std::vector<uint64_t> const &sop, uint32_t const lit_num)
    {
        for (uint64_t lit = 0; lit < lit_num; ++lit)
        {
            uint64_t occur_times = 0;
            for (auto const &cube : sop)
            {
                if (cube_has_lit(cube, lit))
                {
                    ++occur_times;
                }
                if (occur_times > 1)
                { // found
                    return lit;
                }
            }
        }

        // each literal appears once
        return -1;
    }

    /**
     * @brief find the least often occurring literal in the sop
     */
    inline int64_t sop_least_lit(std::vector<uint64_t> const &sop, uint32_t const lit_num)
    {
        int64_t least_lit = -1;
        uint32_t least_times = UINT32_MAX;

        // find the first literal which appears more than once
        for (uint64_t lit = 0; lit < lit_num; ++lit)
        {
            uint32_t occur_times = 0;
            for (auto const &cube : sop)
            {
                if (cube_has_lit(cube, lit))
                {
                    ++occur_times;
                }
            }

            if (occur_times > 1 && occur_times < least_times)
            {
                least_lit = static_cast<int64_t>(lit);
                least_times = occur_times;
            }
        }

        return least_lit;
    }

    /**
     * @brief find the most often occurring literal in the sop
     */
    inline int64_t sop_most_lit(std::vector<uint64_t> const &sop, uint64_t const cube, uint32_t const lit_num)
    {
        int64_t most_lit = -1;
        uint32_t most_times = 1;

        for (uint64_t lit = 0; lit < lit_num; ++lit)
        {
            if (!cube_has_lit(cube, lit))
            {
                continue;
            }
            uint32_t occur_times = 0;
            for (auto const &c : sop)
            {
                if (cube_has_lit(c, lit))
                {
                    ++occur_times;
                }
            }
            if (occur_times > most_times)
            {
                most_lit = static_cast<int64_t>(lit);
                most_times = occur_times;
            }
        }

        return most_lit;
    }

    /**
     * @brief get the most often occurring literal in the sop, and return occur times
     */
    inline bool get_most_lit(std::vector<uint64_t> const &sop, uint64_t &cube, uint32_t const lit)
    {
        uint64_t most_cube = UINT64_MAX;
        uint32_t most_times = 0;

        for (auto const &c : sop)
        {
            if (cube_has_lit(c, lit))
            {
                ++most_times;
                most_cube &= c;
            }
        }

        cube = most_cube;
        return most_times > 1;
    }

    /**
     * @brief get the best literal in the sop
     */
    inline void sop_best_lit(
        std::vector<uint64_t> const &sop, std::vector<uint64_t> &result, uint64_t const cube, uint32_t const lit_num)
    {
        int64_t most_lit = sop_most_lit(sop, cube, lit_num);
        result.push_back(static_cast<uint64_t>(1) << most_lit);
    }

} // namespace detail

/**
 * @brief count num of all literals in the sop
 */
inline uint32_t sop_count_lit(std::vector<uint64_t> const &sop)
{
    uint32_t lits_count = 0;

    for (auto const &c : sop)
    {
        lits_count += detail::cube_count_lit(c);
    }

    return lits_count;
}

/**
 * @brief checks for a common cube divisor in checks for a common cube divisor in the sop.If found, the SOP is divided by
 * that cube.
 */
inline void sop_make_cube_free(std::vector<uint64_t> &sop)
{
    /* find common cube */
    uint64_t mask = UINT64_MAX;
    for (auto const &c : sop)
    {
        mask &= c;
    }
    if (mask == 0)
        return;
    /* make cube free */
    for (auto &c : sop)
    {
        c &= ~mask;
    }
}

/**
 * @brief checks for a common cube divisor in the SOP.
 */
inline bool sop_is_cube_free(std::vector<uint64_t> const &sop)
{
    /* find common cube */
    uint64_t mask = UINT64_MAX;
    for (auto const &c : sop)
    {
        mask &= c;
    }

    return mask == 0;
}

/**
 * @brief divides a SOP by a literal
 */
inline void sop_divide_by_lit(std::vector<uint64_t> &sop, uint64_t const lit)
{
    uint32_t p = 0;
    for (uint32_t i = 0; i < sop.size(); ++i)
    {
        if (detail::cube_has_lit(sop[i], lit))
        {
            sop[p++] = sop[i] & (~(static_cast<uint64_t>(1) << lit));
        }
    }

    sop.resize(p);
}

/*! \brief Algebraic division by a cube
 * This method divides a SOP (divident) by the divisor
 * and stores the resulting quotient and reminder.
 */
inline void sop_divide_by_cube(std::vector<uint64_t> const &divident,
                               std::vector<uint64_t> const &divisor,
                               std::vector<uint64_t> &quotient,
                               std::vector<uint64_t> &reminder)
{
    assert(divisor.size() == 1);

    quotient.clear();
    reminder.clear();

    for (auto const &c : divident)
    {
        if ((c & divisor[0]) == divisor[0])
        {
            quotient.push_back(c & (~divisor[0]));
        }
        else
        {
            reminder.push_back(c);
        }
    }
}

/*! \brief Algebraic division by a cube
 * This method divides a SOP (divident) by the divisor
 * and stores the resulting quotient.
 */
inline void sop_divide_by_cube_no_reminder(std::vector<uint64_t> const &divident,
                                           uint64_t const &divisor,
                                           std::vector<uint64_t> &quotient)
{
    quotient.clear();

    for (auto const &c : divident)
    {
        if ((c & divisor) == divisor)
        {
            quotient.push_back(c & ~divisor);
        }
    }
}

/*! \brief Algebraic division
 *
 * This method divides a SOP (divident) by the divisor
 * and stores the resulting quotient and reminder.
 */
inline void sop_divide(std::vector<uint64_t> &divident,
                       std::vector<uint64_t> const &divisor,
                       std::vector<uint64_t> &quotient,
                       std::vector<uint64_t> &reminder)
{
    /* divisor contains a single cube */
    if (divisor.size() == 1)
    {
        sop_divide_by_cube(divident, divisor, quotient, reminder);
        return;
    }

    quotient.clear();
    reminder.clear();

    /* perform division */
    for (uint32_t i = 0; i < divident.size(); ++i)
    {
        auto const c = divident[i];

        /* cube has been already covered */
        if (detail::cube_has_lit(c, 63))
            continue;

        uint32_t div_i;
        for (div_i = 0u; div_i < divisor.size(); ++div_i)
        {
            if ((c & divisor[div_i]) == divisor[div_i])
                break;
        }

        /* cube is not divisible -> reminder */
        if (div_i >= divisor.size())
            continue;

        /* extract quotient */
        uint64_t c_quotient = c & ~divisor[div_i];

        /* find if c_quotient can be obtained for all the divisors */
        bool found = true;
        for (auto const &div : divisor)
        {
            if (div == divisor[div_i])
                continue;

            found = false;
            for (auto const &c2 : divident)
            {
                /* cube has been already covered */
                if (detail::cube_has_lit(c2, 63))
                    continue;

                if (((c2 & div) == div) && (c_quotient == (c2 & ~div)))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                break;
        }

        if (!found)
            continue;

        /* valid divisor, select covered cubes */
        quotient.push_back(c_quotient);

        divident[i] |= static_cast<uint64_t>(1) << 63;
        for (auto const &div : divisor)
        {
            if (div == divisor[div_i])
                continue;

            for (auto &c2 : divident)
            {
                /* cube has been already covered */
                if (detail::cube_has_lit(c2, 63))
                    continue;

                if (((c2 & div) == div) && (c_quotient == (c2 & ~div)))
                {
                    c2 |= static_cast<uint64_t>(1) << 63;
                    break;
                }
            }
        }
    }

    /* add remainder */
    for (auto &c : divident)
    {
        if (!detail::cube_has_lit(c, 63))
        {
            reminder.push_back(c);
        }
        else
        {
            /* unmark */
            c &= ~(static_cast<uint64_t>(1) << 63);
        }
    }
}

/*! \brief Extracts all the kernels
 * This method is used to identify and collect all
 * the kernels.
 */
inline void sop_kernels_rec(std::vector<uint64_t> const &sop,
                            std::vector<std::vector<uint64_t>> &kernels,
                            uint32_t const j,
                            uint32_t const lit_num)
{
    std::vector<uint64_t> kernel;

    for (uint32_t i = j; i < lit_num; ++i)
    {
        uint64_t c;
        if (detail::get_most_lit(sop, c, i))
        {
            /* cube has been visited already */
            if ((c & ((static_cast<uint64_t>(1) << i) - 1)) > 0u)
                continue;

            sop_divide_by_cube_no_reminder(sop, c, kernel);
            sop_kernels_rec(kernel, kernels, i + 1, lit_num);
        }
    }

    kernels.push_back(sop);
}

/*! \brief Extracts the best factorizing kernel
 * This method is used to identify the best kernel
 * according to the algebraic factorization value.
 */
inline uint32_t sop_best_kernel_rec(std::vector<uint64_t> &sop,
                                    std::vector<uint64_t> &kernel,
                                    std::vector<uint64_t> &best_kernel,
                                    uint32_t const j,
                                    uint32_t &best_cost,
                                    uint32_t const lit_num)
{
    std::vector<uint64_t> new_kernel;
    std::vector<uint64_t> quotient;
    std::vector<uint64_t> reminder;

    /* evaluate kernel */
    sop_divide(sop, kernel, quotient, reminder);
    uint32_t division_cost = sop_count_lit(quotient) + sop_count_lit(reminder);
    uint32_t best_fact_cost = sop_count_lit(kernel);

    for (uint32_t i = j; i < lit_num; ++i)
    {
        uint64_t c;
        if (detail::get_most_lit(kernel, c, i))
        {
            /* cube has been already visited */
            if ((c & ((static_cast<uint64_t>(1) << i) - 1)) > 0u)
                continue;

            /* extract the new kernel */
            sop_divide_by_cube(kernel, {c}, new_kernel, reminder);
            uint32_t fact_cost_rec = detail::cube_count_lit(c) + sop_count_lit(reminder);
            uint32_t fact_cost = sop_best_kernel_rec(sop, new_kernel, best_kernel, i + 1, best_cost, lit_num);

            /* compute the factorization value for kernel */
            if ((fact_cost + fact_cost_rec) < best_fact_cost)
                best_fact_cost = fact_cost + fact_cost_rec;
        }
    }

    if (best_kernel.empty() || (division_cost + best_fact_cost) < best_cost)
    {
        best_kernel = kernel;
        best_cost = division_cost + best_fact_cost;
    }

    return best_fact_cost;
}

/*! \brief Finds a good divisor for a SOP
 * This method is used to identify and return a good
 * divisor for the SOP.
 */
inline bool sop_good_divisor(std::vector<uint64_t> &sop, std::vector<uint64_t> &res, uint32_t const lit_num)
{
    if (sop.size() <= 1)
        return false;

    /* each literal appears no more than once */
    if (detail::sop_lit_occur(sop, lit_num) < 0)
        return false;

    std::vector<uint64_t> kernel = sop;

    /* compute all the kernels and return the one with the best factorization value */
    uint32_t best_cost = 0;
    sop_best_kernel_rec(sop, kernel, res, 0, best_cost, lit_num);

    return true;
}

/*! \brief Translates cubes into products
 * This method translate SOP of kitty::cubes (bits + mask)
 * into SOP of products represented by literals.
 * Example for: ab'c*
 *   - cube: _bits = 1010; _mask = 1110
 *   - product: 10011000
 */
inline std::vector<uint64_t> cubes_to_sop(std::vector<kitty::cube> const &cubes, uint32_t const num_vars)
{
    using sop_t = std::vector<uint64_t>;

    sop_t sop(cubes.size());

    /* Represent literals instead of variables as a a' b b' */
    /* Bit 63 is reserved, up to 31 varibles support */
    auto it = sop.begin();
    for (auto const &c : cubes)
    {
        uint64_t &product = *it++;
        for (uint32_t i = 0; i < num_vars; ++i)
        {
            if (c.get_mask(i))
                product |= static_cast<uint64_t>(1) << (2 * i + static_cast<uint64_t>(c.get_bit(i)));
        }
    }

    return sop;
}

template <class Ntk>
class sop_factoring
{
public:
    using signal = typename Ntk::signal;
    using sop_t = std::vector<uint64_t>;

public:
    sop_factoring() = default;

public:
    template <typename LeavesIterator, typename Fn>
    void operator()(Ntk &dest, kitty::dynamic_truth_table const &function, LeavesIterator begin, LeavesIterator end, Fn &&fn)
        const
    {
        assert(function.num_vars() <= 31);

        if (kitty::is_const0(function))
        {
            /* constant 0 */
            fn(dest.get_constant(false));
            return;
        }
        else if (kitty::is_const0(~function))
        {
            /* constant 1 */
            fn(dest.get_constant(true));
            return;
        }

        /* derive ISOP */
        bool negated;
        auto cubes = get_isop(function, negated);

        /* create literal form of SOP */
        sop_t sop = cubes_to_sop(cubes, function.num_vars());

        /* derive the factored form */
        signal f = gen_factor_rec(dest, {begin, end}, sop, 2 * function.num_vars());

        fn(negated ? !f : f);
    }

private:
    std::vector<kitty::cube> get_isop(kitty::dynamic_truth_table const &function, bool &negated) const
    {
        std::vector<kitty::cube> cubes = kitty::isop(function);

        std::vector<kitty::cube> n_cubes = kitty::isop(~function);

        if (n_cubes.size() < cubes.size())
        {
            negated = true;
            return n_cubes;
        }
        else if (n_cubes.size() == cubes.size())
        {
            uint32_t n_lit = 0;
            uint32_t lit = 0;
            for (auto const &c : n_cubes)
            {
                n_lit += c.num_literals();
            }
            for (auto const &c : cubes)
            {
                lit += c.num_literals();
            }

            if (n_lit < lit)
            {
                negated = true;
                return n_cubes;
            }
        }

        negated = false;
        return cubes;
    }

    signal gen_factor_rec(Ntk &ntk, std::vector<signal> const &children, sop_t &sop, uint32_t const num_lit) const
    {
        sop_t divisor, quotient, reminder;

        assert(sop.size());

        /* compute the divisor */
        if (!sop_good_divisor(sop, divisor, num_lit))
        {
            /* generate trivial sop circuit */
            return gen_andor_circuit_rec(ntk, children, sop.begin(), sop.end(), num_lit);
        }

        /* divide the SOP by the divisor */
        sop_divide(sop, divisor, quotient, reminder);

        assert(quotient.size() > 0);

        if (quotient.size() == 1)
        {
            return lit_factor_rec(ntk, children, sop, quotient[0], num_lit);
        }
        sop_make_cube_free(quotient);

        /* divide the SOP by the quotient */
        sop_divide(sop, quotient, divisor, reminder);

        if (sop_is_cube_free(divisor))
        {
            signal div_s = gen_factor_rec(ntk, children, divisor, num_lit);
            signal quot_s = gen_factor_rec(ntk, children, quotient, num_lit);

            /* build (D)*(Q) + R */
            signal dq_and = ntk.create_and(div_s, quot_s);

            if (reminder.size())
            {
                signal rem_s = gen_factor_rec(ntk, children, reminder, num_lit);
                return ntk.create_or(dq_and, rem_s);
            }

            return dq_and;
        }

        /* get the common cube */
        uint64_t cube = UINT64_MAX;
        for (auto const &c : divisor)
        {
            cube &= c;
        }

        return lit_factor_rec(ntk, children, sop, cube, num_lit);
    }

    signal lit_factor_rec(Ntk &ntk,
                          std::vector<signal> const &children,
                          sop_t const &sop,
                          uint64_t const c_sop,
                          uint32_t const num_lit) const
    {
        sop_t divisor, quotient, reminder;

        /* extract the best literal */
        detail::sop_best_lit(sop, divisor, c_sop, num_lit);

        /* divide SOP by the literal */
        sop_divide_by_cube(sop, divisor, quotient, reminder);

        /* create the divisor: cube */
        signal div_s = gen_and_circuit_rec(ntk, children, divisor[0], 0, num_lit);

        /* factor the quotient */
        signal quot_s = gen_factor_rec(ntk, children, quotient, num_lit);

        /* build l*Q + R */
        signal dq_and = ntk.create_and(div_s, quot_s);

        /* factor the reminder */
        if (reminder.size() != 0)
        {
            signal rem_s = gen_factor_rec(ntk, children, reminder, num_lit);
            return ntk.create_or(dq_and, rem_s);
        }

        return dq_and;
    }

    signal gen_and_circuit_rec(Ntk &ntk,
                               std::vector<signal> const &children,
                               uint64_t const cube,
                               uint32_t const begin,
                               uint32_t const end) const
    {
        /* count set literals */
        uint32_t num_lit = 0;
        uint32_t lit = begin;
        uint32_t i;
        for (i = begin; i < end; ++i)
        {
            if (detail::cube_has_lit(cube, i))
            {
                ++num_lit;
                lit = i;
            }
        }

        assert(num_lit > 0);

        if (num_lit == 1)
        {
            /* return the coprresponding signal with the correct polarity */
            if (lit % 2 == 1)
                return children[lit / 2];
            else
                return !children[lit / 2];
        }

        /* find splitting point */
        uint32_t count_lit = 0;
        for (i = begin; i < end; ++i)
        {
            if (detail::cube_has_lit(cube, i))
            {
                if (count_lit >= num_lit / 2)
                    break;

                ++count_lit;
            }
        }

        signal tree1 = gen_and_circuit_rec(ntk, children, cube, begin, i);
        signal tree2 = gen_and_circuit_rec(ntk, children, cube, i, end);

        return ntk.create_and(tree1, tree2);
    }

    signal gen_andor_circuit_rec(Ntk &ntk,
                                 std::vector<signal> const &children,
                                 sop_t::const_iterator const &begin,
                                 sop_t::const_iterator const &end,
                                 uint32_t const num_lit) const
    {
        auto num_prod = std::distance(begin, end);

        assert(num_prod > 0);

        if (num_prod == 1)
            return gen_and_circuit_rec(ntk, children, *begin, 0, num_lit);

        /* create or tree */
        signal tree1 = gen_andor_circuit_rec(ntk, children, begin, begin + num_prod / 2, num_lit);
        signal tree2 = gen_andor_circuit_rec(ntk, children, begin + num_prod / 2, end, num_lit);

        return ntk.create_or(tree1, tree2);
    }
};

iFPGA_NAMESPACE_HEADER_END
