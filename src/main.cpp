#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

struct ModelInput
{
    double start_army_size;             // > 0          Tamanho inicial do exército
    double start_enemy_size;            // > 0          Tamanho inicial do exército inimigo
    double loose_battle_fraction;       // 0 < x < 1:   Porcentagem do exército que define derrota
    double army_skill;                  // 0 < x <= 1   Perícia do exército em eliminar inimigos
    double enemy_skill;                 // 0 < x <= 1   Perícia do inimigo em eliminar o exército
    double start_ammo;                  // > 0          Quantidade de munição que estará disponível durante a batalha
    double army_fire_rate;              // 0 < x <= 1   Taxa com que o exército consegue atirar a munição disponível
    double ammo_diffusion_coeffient;    // > 0          Velocidade com o que a munição é distribuída para o exército
    double formation_size;              // > 0          Tamanho da formação utilizada na batalha pelo exército
    double front_line_fraction;         // 0 < x <= 1   Porcentagem do exército que atuará na linha de frente
    double enemy_front_line_fraction;   // 0 < x <= 1   Porcentagem do exército inimigo que atuará na linha de frente

    double delta_time;                  // > 0
    double delta_x;                     // > 0

    /**
     * @brief ModelInputData default input data
     */
    ModelInput()
    {
        start_army_size             = 500.0;
        start_enemy_size            = 500.0;
        loose_battle_fraction       = 0.01;
        army_skill                  = 0.03;
        enemy_skill                 = 0.01;
        start_ammo                  = 1000;
        army_fire_rate              = 0.05;
        ammo_diffusion_coeffient    = 0.8;
        formation_size              = 7;
        front_line_fraction         = 0.1;
        enemy_front_line_fraction   = 0.1;

        delta_time                  = 0.07;
        delta_x                     = 0.6;

        std::cout << "CFL = " << ammo_diffusion_coeffient * delta_time / (delta_x * delta_x) << std::endl;
    }


    friend std::ostream& operator<< (std::ostream& out, const ModelInput& input_data)
    {
        out << "#-------------------------------------------------" << std::endl;
        out << "#--- GentlesmanBattle Input Data" << std::endl;
        out << "#-------------------------------------------------" << std::endl;
        out << "#- Start Army Size    : " << input_data.start_army_size << std::endl;
        out << "#- Start Enemy Size   : " << input_data.start_enemy_size << std::endl;
        out << "#- Army Skill         : " << input_data.army_skill << std::endl;
        out << "#- Enemy Skill        : " << input_data.enemy_skill << std::endl;
        out << "#- Start Ammo         : " << input_data.start_ammo << std::endl;
        out << "#- Ammo Diffusion Coef: " << input_data.ammo_diffusion_coeffient << std::endl;
        out << "#- Battle Field Size  : " << input_data.formation_size << std::endl;
        out << "#- Front Line Fraction: " << input_data.front_line_fraction << std::endl;
        out << "#- Delta Time         : " << input_data.delta_time << std::endl;
        out << "#- Delta X            : " << input_data.delta_x << std::endl;
        out << "#--------------------------------------------------" << std::endl;

        return out;
    }
};

struct ModelInfo
{
    double time             = 0.0;

    double new_army_size    = 0.0;
    double old_army_size    = 0.0;

    double new_enemy_size   = 0.0;
    double old_enemy_size   = 0.0;

    std::vector<double> new_ammo_amount;
    std::vector<double> old_ammo_amount;

    const ModelInput& model_input;

    double CFL = 0.0;

    ModelInfo(const ModelInput& input_data) :
        model_input(input_data)
    {
    }

    void update_input()
    {
        // Setting up the mesh for the ammo diffusion
        new_ammo_amount.resize(static_cast<int>(model_input.formation_size / model_input.delta_x));
        old_ammo_amount.resize(new_ammo_amount.size());

        // Initial condition
        old_army_size = model_input.start_army_size;
        old_enemy_size = model_input.start_enemy_size;

        // Ammot starts at rear-line
        old_ammo_amount[1] = model_input.start_ammo;

        CFL = model_input.ammo_diffusion_coeffient * model_input.delta_time / (model_input.delta_x * model_input.delta_x);
    }

    void advance_time()
    {
        // Calculates the fraction of the army and enemies currently standing in the front-line
        double front_line_size = model_input.front_line_fraction * old_army_size;
        double enemy_front_line_size = model_input.front_line_fraction * old_enemy_size;

        // Army
        new_army_size = old_army_size - model_input.delta_time * model_input.enemy_skill * front_line_size * enemy_front_line_size;
        old_army_size = new_army_size;        

        // Enemy
        double shoots_fired = old_ammo_amount.back() * model_input.army_fire_rate;
        new_enemy_size = old_enemy_size - model_input.delta_time * model_input.army_skill * shoots_fired * front_line_size * enemy_front_line_size;
        old_enemy_size = new_enemy_size;

        // Ammo Diffusion        
        for(size_t i = 1; i < new_ammo_amount.size() - 1; ++i)
        {
            new_ammo_amount[i] = old_ammo_amount[i] + CFL * (old_ammo_amount[i-1] - 2.0 * old_ammo_amount[i] + old_ammo_amount[i+1]);
        }

        //
        // Ammo boundary conditions
        //

        // At x=0 there is no flow.
        new_ammo_amount[0] = new_ammo_amount[1];

        // At x=L the ammo is being used by the soldiers
        // Calculates the percentage of ammo used at the frontline.
        double ammo_usage_ratio = 1.0 - (1.0 / front_line_size);
        new_ammo_amount.back() = new_ammo_amount[new_ammo_amount.size() - 2] * ammo_usage_ratio;

        // Swap vectors for next time step
        new_ammo_amount.swap(old_ammo_amount);

        // Finally, advance the time
        time += model_input.delta_time;
    }

    /**
     * @brief True if the battle has came to an end
     *
     * The condition for the battle to stop is when the army size reaches a fraction defined
     * by @ref ModelInputData::loose_battle_fraction. The condition is applied for both the army
     * and the enemies.
     *
     * @return True if the battle has came to an end false otherwise
     */
    bool should_stop() const
    {        
        return new_army_size <= model_input.start_army_size * model_input.loose_battle_fraction ||
                new_enemy_size <= model_input.start_enemy_size * model_input.loose_battle_fraction;

    }

    /**
     * @brief Returns true if the number of soldiers is bigger than the number of enemies soldiers at this moment
     */
    bool is_army_winning() const
    {
        return old_army_size > old_enemy_size;
    }

    friend std::ostream& operator<< (std::ostream& out, const ModelInfo& info)
    {
        out << "#-------------------------------------------------" << std::endl;
        out << "#--- GentlesmanBattle Execution Summary" << std::endl;
        out << "#-------------------------------------------------" << std::endl;
        out << "# Number of Iterations      : " << info.time / info.model_input.delta_time << std::endl;
        out << "# Total time                : " << info.time << std::endl;
        out << "# Soldiers Count            : " << info.new_army_size << std::endl;
        out << "# Enemies Count             : " << info.new_enemy_size << std::endl;
        out << "# Available Ammo at Fronline: " << info.new_ammo_amount.back() << std::endl;
        out << "# Available Ammo at Rearline: " << info.new_ammo_amount.front() << std::endl;
        out << "#-------------------------------------------------" << std::endl;
        out << "# Battle Result: ";

        if(info.is_army_winning())
        {
            out << "YOU WIN!" << std::endl;
        }
        else
        {
            out << "YOU LOOSE!" << std::endl;
        }

        out << "#-------------------------------------------------" << std::endl;

        return out;
    }    
};

struct ModelOutput
{
    std::fstream output_file;
    std::fstream phase_plane_file;

    const ModelInput& model_input;
    const ModelInfo& model_info;

    const std::string prefix;

    ModelOutput(const ModelInfo& _model_info, const ModelInput& _model_input, const std::string& _prefix = "")
        : model_input(_model_input),
          model_info(_model_info),
          prefix(_prefix)
    {        
    }

    void start_execution()
    {
        output_file.open(prefix + "_gentlemans_battle.dat", std::ios::out);
        output_file << model_input << std::endl << std::endl;
        output_file << "# Time  Army    Enemy   Rearguard   Frontline  " << std::endl;
    }

    void write_output_step()
    {
        output_file << std::setw(10) << std::setprecision(10) << std::fixed << std::setfill('0') <<
                       model_info.time << " " <<
                       model_info.old_army_size << " " <<
                       model_info.old_enemy_size << " " <<
                       model_info.old_ammo_amount.front() << " " <<
                       model_info.old_ammo_amount.back() << " " <<
                       std::endl;
    }

    void stop_execution(bool b_show_plots = true)
    {
        // Write Execution Summary into the output file
        output_file << model_info << std::endl;

        if(output_file.is_open())
        {
            output_file.close();
        }

        if(b_show_plots)
        {
            std::string output_filename_quotes = "'" + prefix + "_gentlemans_battle.dat'";
            std::string gnuplot_reaction_script = prefix + "_gentlemans_battle_result.gnu";
            std::string gnuplot_phase_plane_script = prefix + "_gentlemans_battle_phase_plane.gnu";
            std::string gnuplot_diffusion_script = prefix + "_gentlemans_battle_diffusion.gnu";

            const int title_font_size = 15;

            {
                std::fstream gnuplot_script_file(gnuplot_reaction_script, std::ios::out);

                gnuplot_script_file << "set terminal 'wxt'" << std::endl;
                gnuplot_script_file << "set xlabel 'Tempo'" << std::endl;
                gnuplot_script_file << "set ylabel 'Número de Soldados'" << std::endl;
                gnuplot_script_file << "set zeroaxis" << std::endl;
                gnuplot_script_file << "set yrange [0:550]" << std::endl;
                gnuplot_script_file << "set title 'Evolução do Número de Soldados no Campo de Batalha' font 'Arial, " << title_font_size << "'" << std::endl;
                gnuplot_script_file << "plot " <<
                                       output_filename_quotes << " using 1:2 with lines title 'Soldados'," <<
                                       output_filename_quotes << " using 1:3 with lines title 'Inimigos' linetype rgb '#6f99c8'" << std::endl;
                gnuplot_script_file << "set output 'report/figs/battle_reaction.png'" << std::endl;
                gnuplot_script_file << "set terminal pngcairo enhanced font 'arial,10' fontscale 1.0" << std::endl;
                gnuplot_script_file << "replot" << std::endl;

            }
            {
                std::fstream gnuplot_script_file(gnuplot_diffusion_script, std::ios::out);

                gnuplot_script_file << "set terminal 'wxt'" << std::endl;
                gnuplot_script_file << "set xlabel 'Tempo'" << std::endl;
                gnuplot_script_file << "set ylabel 'Concentração de Munição'" << std::endl;
                gnuplot_script_file << "set zeroaxis" << std::endl;

                gnuplot_script_file << "set title 'Munição nas linhas de frente e retarguarda' font 'Arial, " << title_font_size << "'" << std::endl;
                gnuplot_script_file << "plot " <<
                                       output_filename_quotes << " using 1:4 with lines title 'Retarguarda'," <<
                                       output_filename_quotes << " using 1:5 with lines title 'Linha de frente' linetype rgb '#6f99c8'" << std::endl;
                gnuplot_script_file << "set output 'report/figs/battle_ammo_diffusion.png'" << std::endl;
                gnuplot_script_file << "set terminal pngcairo enhanced font 'arial,10' fontscale 1.0" << std::endl;
                gnuplot_script_file << "replot" << std::endl;
            }
            {
                const ModelInput& input = model_info.model_input;

                std::fstream gnuplot_script_file(gnuplot_phase_plane_script, std::ios::out);

                gnuplot_script_file << "set terminal 'wxt'" << std::endl;

                gnuplot_script_file << "k1 = " << input.enemy_skill << std::endl;
                gnuplot_script_file << "k2 = " << input.army_skill << std::endl;
                gnuplot_script_file << "alpha = " << input.front_line_fraction << std::endl;
                gnuplot_script_file << "beta = " << input.enemy_front_line_fraction << std::endl;
                gnuplot_script_file << "vec_scale = 0.5" << std::endl;

                gnuplot_script_file << "dEdt(I,E) = -k1 * alpha * E * beta * I" << std::endl;
                gnuplot_script_file << "dIdt(I,E) = -k2 * alpha * E * beta * I" << std::endl;

                gnuplot_script_file << "vx(x,y) = dEdt(x,y) * vec_scale # * (1 / sqrt(dEdt(x,y)**2 + dIdt(x,y)**2))" << std::endl;
                gnuplot_script_file << "vy(x,y) = dIdt(x,y) * vec_scale # * (1 / sqrt(dEdt(x,y)**2 + dIdt(x,y)**2))" << std::endl;

                gnuplot_script_file << "set samples 20" << std::endl;
                gnuplot_script_file << "set zeroaxis" << std::endl;

                gnuplot_script_file << "set xlabel 'Número de Soldados - E(t)'" << std::endl;
                gnuplot_script_file << "set ylabel 'Número de Inimigos - I(t)'" << std::endl;
                gnuplot_script_file << "set title 'Plano de Fase' font 'Arial, " << title_font_size << "'" << std::endl;

                gnuplot_script_file << "plot '_gentlemans_battle.dat' using 2:3 with lines title 'Número de Soldados x Inimigos'," <<
                                       " '++' u 1:2:(vx($1,$2)):(vy($1,$2)) with vectors notitle linetype rgb '#6f99c8'" <<
                                       std::endl;

                gnuplot_script_file << "set output 'report/figs/battle_phase_plane.png'" << std::endl;
                gnuplot_script_file << "set terminal pngcairo enhanced font 'Arial,10' fontscale 1.0" << std::endl;
                gnuplot_script_file << "replot" << std::endl;
            }

            // show reaction plot
            std::string plot_command = "gnuplot -p " + gnuplot_reaction_script + " > battle_reaction.png";
            std::cout << plot_command << std::endl;
            system(plot_command.data());

            // show diffusion plot
            plot_command = "gnuplot -p " + gnuplot_diffusion_script + " > battle_ammo_diffusion.png";
            std::cout << plot_command << std::endl;
            system(plot_command.data());

            // show phase plane plot
            plot_command = "gnuplot -p " + gnuplot_phase_plane_script + " > battle_phase_plane.png";
            std::cout << plot_command << std::endl;
            system(plot_command.data());
        }
    }
};

struct ModelCondensedOutput
{
    int num_executions;
    std::string param_name;
    std::string graph_title;
    std::string output_filename;
    std::vector<double> param_value_list;

    ModelCondensedOutput(int _num_executions,
                         const std::string& _param_name,
                         const std::string& _graph_title,
                         const std::string& _output_filename
                         ) :
        num_executions(_num_executions),
        param_name(_param_name),
        graph_title(_graph_title),
        output_filename(_output_filename)
    {
        param_value_list.reserve(num_executions);
    }

    void add_param_value(double param_value)
    {
        param_value_list.push_back(param_value);
    }

    void show_condensed_plot()
    {
        std::string script_filename = "_condenser.gnu";


        std::fstream gnuplot_script_file(script_filename, std::ios::out);

        gnuplot_script_file << "set terminal 'wxt'" << std::endl;
        gnuplot_script_file << "set xlabel 'Número de Soldados - E(t)'" << std::endl;
        gnuplot_script_file << "set ylabel 'Número de Inimigos - I(t)" << std::endl;
        gnuplot_script_file << "set title '" << graph_title << "' font 'Arial, 15'" << std::endl;

        gnuplot_script_file << "plot ";

        for(int i = 0; i < num_executions; ++i)
        {
            gnuplot_script_file << "'" << i << "_gentlemans_battle.dat' using 2:3 with lines title '" << param_name << " = " << param_value_list[i] << "',";
        }
        gnuplot_script_file << std::endl;

        gnuplot_script_file << "set terminal pngcairo enhanced font 'Arial, 10' fontscale 1.0" << std::endl;
        gnuplot_script_file << "set output 'report/figs/battle_" << output_filename << ".png'" << std::endl;
        gnuplot_script_file << "replot" << std::endl;

        gnuplot_script_file.close();

        std::string plot_command = "gnuplot -p " + script_filename;
        std::cout << plot_command << std::endl;

        system(plot_command.data());
    }
};

struct GentlesmanBattleModel
{
    ModelInput input;
    ModelOutput output;
    ModelInfo info;

    GentlesmanBattleModel(const std::string& prefix = "") :
        output(info, input, prefix), info(input)
    {
    }

    void run(bool b_show_plots = true)
    {        
        // Print parameters information
        std::cout << input << std::endl;

        info.update_input();

        output.start_execution();

        do
        {
            output.write_output_step();
            info.advance_time();
        }
        while(!info.should_stop());

        // Print execution summary
        std::cout << info << std::endl;

        output.stop_execution(b_show_plots);
    }
};

int main()
{
    const int num_executions = 1;
    std::string param_name = "L";

    double param_min = 2;
    double param_max = 20;

    if(num_executions > 1)
    {
        ModelCondensedOutput condensend_output(num_executions,
                                               param_name,
                                               "Resultado da Batalha com Variação no Espaçamento da Formação",
                                               "formation_size_variation");

        for(int i = 0; i < num_executions; ++i)
        {
            GentlesmanBattleModel model(std::to_string(i));
            double param_value = param_min + (param_max - param_min) * (i / static_cast<double>(num_executions));

            model.input.formation_size = param_value;

            model.run(false);

            condensend_output.add_param_value(model.input.formation_size);
        }
        condensend_output.show_condensed_plot();
    }
    else
    {
        GentlesmanBattleModel model;
        model.run();
    }

    return EXIT_SUCCESS;
}

