namespace FastTravelTweaks {
    class Config {
    public:
        static void LoadConfig() noexcept;

        inline static bool fasterIfOwnHorse{};
        inline static float normalSpeed{};
        inline static float horsebackSpeed{};
        inline static float cartSpeed{};

    };
}  // namespace FastTravelTweaks