#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

struct ModelInputData
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
    ModelInputData()
    {
        start_army_size             = 500.0;
        start_enemy_size            = 500.0;
        loose_battle_fraction       = 0.01;
        army_skill                  = 0.03;
        enemy_skill                 = 0.03;
        start_ammo                  = 950;
        army_fire_rate              = 0.05;
        ammo_diffusion_coeffient    = 1.0;
        formation_size              = 7;
        front_line_fraction         = 0.1;
        enemy_front_line_fraction   = 1.0;

        delta_time                  = 0.07;
        delta_x                     = 0.6;

        std::cout << "CFL = " << ammo_diffusion_coeffient * delta_time / (delta_x * delta_x) << std::endl;
    }


    friend std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data)
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

    const ModelInputData& model_input;

    const double CFL = model_input.ammo_diffusion_coeffient * model_input.delta_time / (model_input.delta_x * model_input.delta_x);

    ModelInfo(const ModelInputData& input_data) :
        model_input(input_data)
    {
        // Setting up the mesh for the ammo diffusion
        new_ammo_amount.resize(static_cast<int>(input_data.formation_size / input_data.delta_x));
        old_ammo_amount.resize(new_ammo_amount.size());

        // Initial condition
        old_army_size = input_data.start_army_size;
        old_enemy_size = input_data.start_enemy_size;

        // Ammot starts at rear-line
        old_ammo_amount[1] = model_input.start_ammo;
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
    std::string model_output_filename;
    std::fstream output_file;
    const ModelInfo& model_info;

    ModelOutput(const std::string& prefix, const ModelInfo& _model_info, const ModelInputData& model_input)
        : model_output_filename(prefix + "_gentlemans_battle.dat"),
          model_info(_model_info)
    {
        output_file.open(model_output_filename, std::ios::out);
        output_file << model_input << std::endl << std::endl;
        output_file << "# Time  ArmySize    EnemySize" << std::endl;
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

    void stop_execution(bool b_show_gnuplot = true)
    {
        // Write Execution Summary into the output file
        output_file << model_info << std::endl;

        if(output_file.is_open())
        {
            output_file.close();
        }

        if(b_show_gnuplot)
        {
            std::string output_filename_quotes = "'" + model_output_filename + "'";
            std::string gnuplot_script_filename = "_gentlemans_battle.gnu";

            std::fstream gnuplot_script_file(gnuplot_script_filename, std::ios::out);

            gnuplot_script_file << "set xlabel 'Time'" << std::endl;
            gnuplot_script_file << "set ylabel";

            std::string plot_command = "gnuplot -p -e \"plot " +
                    output_filename_quotes + " using 1:2 with lines title 'Army'," +
                    output_filename_quotes + " using 1:3 with lines title 'Enemies'," +
                    output_filename_quotes + "using 1:4 with lines title 'Ammo at rearguard'," +
                    output_filename_quotes + "using 1:5 with lines title 'Ammo at frontline'" +
                    "\"";

            std::cout << plot_command << std::endl;

            // show reaction plot
            system(plot_command.data());

            // show diffusion plot
            plot_command = "gnuplot -p -e \"plot " +
                    output_filename_quotes + "using 1:4 with lines title 'Ammo at rearguard'," +
                    output_filename_quotes + "using 1:5 with lines title 'Ammo at frontline'" +
                    "\"";
//            system(plot_command.data());
            std::cout << plot_command << std::endl;
        }
    }
};

struct GentlesmanBattleModel
{
    ModelInputData input;
    ModelOutput output;
    ModelInfo info;

    GentlesmanBattleModel() :
        output("", info, input), info(input)
    {
    }

    void run()
    {
        // Print parameters information
        std::cout << input << std::endl;

        do
        {
            output.write_output_step();
            info.advance_time();
        }
        while(!info.should_stop());

        // Print execution summary
        std::cout << info << std::endl;
    }
};

int main()
{
    GentlesmanBattleModel model;
    model.run();
    model.output.stop_execution();
    return EXIT_SUCCESS;
}

