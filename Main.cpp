# include <Siv3D.hpp>

// シーンの名前
enum class State
{
	Title,
	Game
};

// ゲームデータ
struct GameData
{
	// ハイスコア
	int32 highScore = 0;
};

// シーン管理クラス
using MyApp = SceneManager<State, GameData>;

// タイトルシーン
class Title : public MyApp::Scene
{
private:

	Rect m_startButton = Rect(Arg::center = Scene::Center().movedBy(0, 0), 300, 60);
	Transition m_startTransition = Transition(0.4s, 0.2s);

	Rect m_exitButton = Rect(Arg::center = Scene::Center().movedBy(0, 100), 300, 60);
	Transition m_exitTransition = Transition(0.4s, 0.2s);

public:

	Title(const InitData& init)
		: IScene(init) {}

	void update() override
	{
		m_startTransition.update(m_startButton.mouseOver());
		m_exitTransition.update(m_exitButton.mouseOver());

		if (m_startButton.mouseOver() || m_exitButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (m_startButton.leftClicked())
		{
			changeScene(State::Game);
		}

		if (m_exitButton.leftClicked())
		{
			System::Exit();
		}
	}

	void draw() const override
	{
		const String titleText = U"Polyomino Bridge";
		const Vec2 center(Scene::Center().x, 120);
		FontAsset(U"Title")(titleText).drawAt(center.movedBy(4, 6), ColorF(0.0, 0.5));
		FontAsset(U"Title")(titleText).drawAt(center);

		m_startButton.draw(ColorF(1.0, m_startTransition.value())).drawFrame(2);
		m_exitButton.draw(ColorF(1.0, m_exitTransition.value())).drawFrame(2);

		FontAsset(U"Menu")(U"はじめる").drawAt(m_startButton.center(), ColorF(0.25));
		FontAsset(U"Menu")(U"おわる").drawAt(m_exitButton.center(), ColorF(0.25));

		Rect(0, 500, Scene::Width(), Scene::Height() - 500)
			.draw(Arg::top = ColorF(0.0, 0.0), Arg::bottom = ColorF(0.0, 0.5));

		const int32 highScore = getData().highScore;
		FontAsset(U"Score")(U"High score: {}"_fmt(highScore)).drawAt(Vec2(620, 550));
	}
};

class Polyomino
{
private:
	Array<Point> m_points;
	Point m_topLeft;
	Point m_size;

public:

	Polyomino(int32 number)
	{
		Point point(0, 0);
		m_points = { point };
		Array<Point> placablePoints = { point };
		size_t pointIndex = 0;

		for (int32 i = 0; i < number - 1; i++)
		{
			Array<Point> movedPoints = { point.movedBy(0, -1), point.movedBy(-1, 0), point.movedBy(1, 0), point.movedBy(0, 1) };
			for (const Point& movedPoint : movedPoints)
			{
				if (isPlacable(movedPoint) && !placablePoints.includes(movedPoint))
				{
					placablePoints << movedPoint;
				}
			}
			pointIndex = Random(pointIndex + 1, placablePoints.size() - 1);
			point = placablePoints[pointIndex];
			m_points << point;
		}
		setTopLeft0();
		m_size = getBottomRight();
		m_size.x += 1;
		m_size.y += 1;
	}

	const Array<Point>& points() const
	{
		return m_points;
	}

	const Point& size() const
	{
		return m_size;
	}

	void setTopLeft0()
	{
		const Point topLeft = getTopLeft();
		m_points.each([&topLeft](Point& p) { p -= topLeft; });
	}

	const Point& getTopLeft() const
	{
		const int32 minX = m_points.reduce([](int32 x, const Point& p) { return Min(x, p.x); }, 0);
		const int32 minY = m_points.reduce([](int32 y, const Point& p) { return Min(y, p.y); }, 0);
		return { minX, minY };
	}

	const Point& getBottomRight() const
	{
		const int32 maxX = m_points.reduce([](int32 x, const Point& p) { return Max(x, p.x); }, 0);
		const int32 maxY = m_points.reduce([](int32 y, const Point& p) { return Max(y, p.y); }, 0);
		return { maxX, maxY };
	}

	bool isPlacable(const Point& point) const
	{
		return point.y > 0 || point.y == 0 && point.x >= 0;
	}

	void rotateRight()
	{
		Array<Point> rotatedPoints;
		for (const Point& p : m_points)
		{
			rotatedPoints << Point(-p.y, p.x);
		}
		m_points = rotatedPoints;
		setTopLeft0();
		std::swap(m_size.x, m_size.y);
	}

	void rotateLeft()
	{
		Array<Point> rotatedPoints;
		for (const Point& p : m_points)
		{
			rotatedPoints << Point(p.y, -p.x);
		}
		m_points = rotatedPoints;
		setTopLeft0();
		std::swap(m_size.x, m_size.y);
	}
};

// ゲームシーン
class Game : public MyApp::Scene
{
private:

	static constexpr int32 m_maxX = 12;
	static constexpr int32 m_height = 10;

	Array<Point> m_bridge;
	Array<int32> m_bridgeYs;
	Point m_rectSize;
	Polyomino m_polyomino;
	Point m_pos;
	int32 m_direction;
	Duration m_period;
	Stopwatch m_stopwatch;
	bool m_isReverse;

	// スコア
	int32 m_score = 0;

public:

	Game(const InitData& init)
		: IScene(init)
		, m_rectSize(Scene::Height() / m_height, Scene::Height() / m_height)
		, m_polyomino(4)
		, m_pos(0, -m_polyomino.size().y)
		, m_direction(1)
		, m_period(SecondsF(.4))
		, m_stopwatch(true)
		, m_isReverse(false)
	{
	}

	void update() override
	{
		if (KeyLeft.down())
		{
			m_polyomino.rotateLeft();
		}
		if (KeyRight.down())
		{
			m_polyomino.rotateRight();
		}

		Duration sF = m_stopwatch.elapsed();
		while (sF > m_period)
		{
			sF -= m_period;
			m_pos.y += m_direction;
		}
		m_stopwatch.set(sF);

		if (m_direction == -1 && m_pos.y <= -m_polyomino.size().y || m_direction == 1 && m_pos.y >= m_height)
		{
			if (m_isReverse)
			{
				m_direction *= -1;
			}
			else
			{
				m_pos.y -= (m_polyomino.size().y + m_height) * m_direction;
			}
		}
		if (KeySpace.down())
		{
			bool gameover = false;
			if (m_bridgeYs)
			{
				Array<int32> polyominoYs = m_polyomino.points().filter([](const Point& p) { return p.x == 0; }).map([this](const Point& p) { return m_pos.y + p.y; });
				polyominoYs.sort();
				Array<int32> intersection;
				std::set_intersection(m_bridgeYs.begin(), m_bridgeYs.end(), polyominoYs.begin(), polyominoYs.end(), std::inserter(intersection, intersection.end()));
				if (!intersection)
				{
					gameover = true;
					changeScene(State::Title);
					getData().highScore = Max(getData().highScore, m_score);
				}
			}
			if (!gameover)
			{
				m_score += m_polyomino.size().x;
				Array<Point> newPoints = m_polyomino.points().map([this](const Point& p) { return m_pos + p; });
				m_bridge.insert(m_bridge.end(), newPoints.begin(), newPoints.end());
				int32 maxX = m_polyomino.getBottomRight().x;
				m_bridgeYs = m_polyomino.points().filter([maxX](const Point& p) { return p.x == maxX; }).map([this](const Point& p) { return m_pos.y + p.y; });
				m_bridgeYs.sort();
				m_pos = Point(m_pos.x + m_polyomino.size().x, -m_polyomino.size().y);
				m_polyomino = Polyomino(Random(4, 8));
				m_direction = 1;
				m_period = SecondsF(.5 / sqrt(m_score));
				m_isReverse = RandomBool();
			}
		}
	}

	void draw() const override
	{
		Mat3x2 mat = Mat3x2::Identity();
		int32 left = m_pos.x + Max(m_polyomino.size().x, m_polyomino.size().y);
		if (left > m_maxX)
		{
			mat = Mat3x2::Translate((m_maxX - left) * m_rectSize.x, 0);
		}
		{
			const Transformer2D t(mat);
			for (const Point& point : m_bridge)
			{
				Rect(point * m_rectSize, m_rectSize).draw(Palette::Orange);
			}
			for (const Point& point : m_polyomino.points())
			{
				Rect((m_pos + point) * m_rectSize, m_rectSize).draw(Palette::Yellow);
			}
		}
		FontAsset(U"Score")(U"Score: {}"_fmt(m_score)).drawAt(Vec2(620, 500));
	}

};

void Main()
{
	// 使用するフォントアセットを登録
	FontAsset::Register(U"Title", 80, Resource(U"font/PixelMplus12-Bold.ttf"));
	FontAsset::Register(U"Menu", 30, Resource(U"font/PixelMplus10-Regular.ttf"));
	FontAsset::Register(U"Score", 36, Resource(U"font/PixelMplus10-Bold.ttf"));

	// 背景色を設定
	Scene::SetBackground(Palette::Mediumpurple);

	Window::SetTitle(U"Polyomino Bridge");

	// シーンと遷移時の色を設定
	MyApp manager;
	manager
		.add<Title>(State::Title)
		.add<Game>(State::Game)
		.setFadeColor(ColorF(1.0));

	while (System::Update())
	{
		if (!manager.update())
		{
			break;
		}
	}
}