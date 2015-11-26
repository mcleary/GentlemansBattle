
#include <iostream>
#include <vector>

struct ModelInputData
{
    double start_army_size;             // > 0
    double start_enemy_size;            // > 0
    double army_skill;                  // 0 < x <= 1
    double enemy_skill;                 // 0 < x <= 1
    double start_ammo;                  // > 0
    double ammo_diffusion_coeffient;    // > 0
    double battlefield_size;            // > 0
    double front_line_fraction;         // 0 < x <= 1

    double delta_time;                  // > 0
    double delta_x;                     // > 0

    /**
     * @brief ModelInputData default input data
     */
    ModelInputData()
    {
        start_army_size             = 100.0;
        start_enemy_size            = 100.0;
        army_skill                  = 0.5;
        enemy_skill                 = 0.5;
        start_ammo                  = 1000;
        ammo_diffusion_coeffient    = 1.0;
        battlefield_size            = 10;
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

    ModelInfo(const ModelInputData& input_data)
    {
        old_army_size = input_data.start_army_size;
        old_enemy_size = input_data.start_enemy_size;

        new_ammo_amount.resize(input_data.battlefield_size / input_data.delta_x);
        old_ammo_amount.resize(new_ammo_amount.size());
    }
};

std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data);

int main(int argc, char *argv[])
{
    ModelInputData model_input;
    ModelInfo model_info(model_input);

    std::cout << model_input << std::endl;

    return EXIT_SUCCESS;
}


std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data)
{
    out << "-------------------------------------------------" << std::endl;
    out << "--- GentlesmanBattle Input Data" << std::endl;
    out << "-------------------------------------------------" << std::endl;
    out << "- Start Army Size    : " << input_data.start_army_size << std::endl;
    out << "- Start Enemy Size   : " << input_data.start_enemy_size << std::endl;
    out << "- Army Skill         : " << input_data.army_skill << std::endl;
    out << "- Enemy Skill        : " << input_data.enemy_skill << std::endl;
    out << "- Start Ammo         : " << input_data.start_ammo << std::endl;
    out << "- Ammo Diffusion Coef: " << input_data.ammo_diffusion_coeffient << std::endl;
    out << "- Battle Field Size  : " << input_data.battlefield_size << std::endl;
    out << "- Front Line Fraction: " << input_data.front_line_fraction << std::endl;
    out << "- Delta Time         : " << input_data.delta_time << std::endl;
    out << "- Delta X            : " << input_data.delta_x << std::endl;
    out << "--------------------------------------------------" << std::endl;

    return out;
}

std::ostream& operator<< (std::ostream& out, const ModelInfo& model_info)
{
    return out;
}

