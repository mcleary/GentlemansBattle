
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <limits>

struct ModelInputData
{
    double start_army_size;             // > 0          Tamanho inicial do exército
    double start_enemy_size;            // > 0          Tamanho inicial do exército inimigo
    double loose_battle_fraction;       // 0 < x < 1:   Porcentagem do exército que define derrota
    double army_skill;                  // 0 < x <= 1   Perícia do exército em eliminar inimigos
    double enemy_skill;                 // 0 < x <= 1   Perícia do inimigo em eliminar o exército
    double start_ammo;                  // > 0          Quantidade de munição que estará disponível durante a batalha
    double ammo_diffusion_coeffient;    // > 0          Velocidade com o que a munição é distribuída para o exército
    double formation_size;              // > 0          Tamanho da formação utilizada na batalha pelo exército
    double front_line_fraction;         // 0 < x <= 1   Porcentagem do exército que atuará na linha de frente

    double delta_time;                  // > 0
    double delta_x;                     // > 0

    /**
     * @brief ModelInputData default input data
     */
    ModelInputData()
    {
        start_army_size             = 100.0;
        start_enemy_size            = 100.0;
        loose_battle_fraction       = 0.1;
        army_skill                  = 0.5;
        enemy_skill                 = 0.5;
        start_ammo                  = 1000;
        ammo_diffusion_coeffient    = 1.0;
        formation_size            = 10;
        front_line_fraction         = 0.1;

        delta_time                  = 0.1;
        delta_x                     = 1;
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

    ModelInfo(const ModelInputData& input_data) :
        model_input(input_data)
    {
        old_army_size = input_data.start_army_size;
        old_enemy_size = input_data.start_enemy_size;

        new_ammo_amount.resize(input_data.formation_size / input_data.delta_x);
        old_ammo_amount.resize(new_ammo_amount.size());
    }

    void advance_time(double delta_time)
    {
        double front_line_size = model_input.front_line_fraction * old_army_size;
        double available_ammo = old_ammo_amount.back();

        new_army_size = old_army_size - delta_time * model_input.enemy_skill * front_line_size;
        old_army_size = new_army_size;

        new_enemy_size = old_enemy_size - delta_time * model_input.army_skill * available_ammo * front_line_size * old_enemy_size;
        old_enemy_size = new_enemy_size;



        time += delta_time;
    }

    bool should_stop() const
    {
        return new_army_size <= model_input.start_army_size * model_input.loose_battle_fraction ||
                new_enemy_size <= model_input.start_enemy_size * model_input.loose_battle_fraction;
    }
};

std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data);

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

        if(!output_file.is_open())
        {
            throw std::runtime_error("Cannot open outputfile: " + model_output_filename);;
        }

        output_file << model_input << std::endl << std::endl;
        output_file << "# Time  ArmySize    EnemySize" << std::endl;
    }

    void write()
    {
        output_file << model_info.time << "\t" <<
                       model_info.old_army_size << "\t" <<
                       model_info.old_enemy_size << std::endl;
    }
};


int main(int argc, char *argv[])
{
    ModelInputData model_input;
    ModelInfo model_info(model_input);
    ModelOutput model_output("", model_info, model_input);

    model_info.new_enemy_size = 100;

    do
    {
        model_output.write();
        model_info.advance_time(model_input.delta_time);
    }
    while(!model_info.should_stop());

    return EXIT_SUCCESS;
}


std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data)
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

