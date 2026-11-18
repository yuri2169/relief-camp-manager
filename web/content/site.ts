// Every word on the site lives here. Components contain no strings.
export const site = {
  locale: "en-IN",
  name: "relief camp",
  title: "Relief Camp Manager: offline camp management for the Assam floods",
  description:
    "An offline relief camp manager for flood camps in Assam. It runs on one laptop with no internet, tracks rations and stock, and says where supplies have to move.",
  skipLink: "Skip to content",
  a11y: { mainNav: "Main", appNav: "App" },
  repo: {
    label: "Source on GitHub",
    href: "https://github.com/yuri2169/relief-camp-manager",
  },
  nav: [
    { label: "Problem", href: "/#problem" },
    { label: "How", href: "/#how" },
    { label: "Inside", href: "/#inside" },
  ],
  hero: {
    headline: ["Two days of", "rice in camp 2.", "Eighteen in", "camp 3."],
    lede:
      "A relief camp manager that runs offline on one laptop, and says where the stock has to move before anyone goes short.",
    sticker: { lines: ["live", "demo"], href: "/app" },
  },
  stats: [
    { figure: "2.0", label: "days of rice left in camp 2 when the demo starts" },
    { figure: "0", label: "servers, networks or licences needed to run it" },
    { figure: "1,923", label: "test assertions, passing on two compilers" },
  ],
  problem: {
    title: "A paper register notices too late.",
    paragraphs: [
      "Every monsoon in Assam, camp registers are kept on paper. Stock runs out with no warning because nobody is dividing what is left by how fast it goes. Some families collect twice in a day while others are missed, because a paper register cannot be searched while a queue is waiting.",
      "The people who need serving first — infants, pregnant women, the elderly, the disabled — are served in the order they happen to arrive. And the camp with eighteen days of rice has no way of knowing that the camp an hour away has two.",
    ],
    pull: {
      figure: "88 / 90",
      label: "occupancy at Laharighat when the demo starts: two beds free",
    },
  },
  how: {
    title: "Three things it does on one laptop.",
    steps: [
      {
        title: "Register a family",
        body:
          "A family is a list of people with ages. From the ages it works out the ration each person draws and a vulnerability score for the household, before anyone has queued.",
      },
      {
        title: "Serve the most vulnerable first",
        body:
          "Distribution runs down a queue ordered by that score, and refuses a second claim on the same day. The duplicate is reported, not silently dropped.",
      },
      {
        title: "Move stock before it runs out",
        body:
          "Each camp knows how many days of each item it holds. When one falls below three days, the camp with the most to spare is named, with the exact quantity to move.",
      },
    ],
  },
  demo: {
    title: "Try it with the Morigaon dataset.",
    body:
      "Three camps, forty families, fixed to 14 July 2026 so the figures never drift. Every number comes from the console program's own seed files. Nothing you change is saved.",
    cta: { label: "Open the live demo", short: "Demo", href: "/app" },
    media: {
      src: "/overview.jpg" as string | null,
      alt:
        "The camp overview: one item below three days of supply, the three camps at 2.0, 9.0 and 12.0 days, and a panel proposing 22.2 kg of rice moved from Mayong to Laharighat",
      placeholder: "Screenshot or poster frame goes here, 16:10",
    },
  },
  inside: {
    title: "What it is made of.",
    rows: [
      { label: "Language", value: "C++17, standard library only" },
      { label: "Interface", value: "Console, no GUI toolkit" },
      { label: "Storage", value: "Nine CSV files, written atomically" },
      { label: "Autosave", value: "A background thread, every 30 seconds" },
      { label: "Tests", value: "1,923 assertions, no framework" },
      { label: "Compilers", value: "Apple clang 21 and GNU g++ 15.2.0" },
      { label: "Not tested on", value: "Linux or Windows" },
    ],
  },
  footer: {
    note:
      "The program itself is a C++17 console application that runs offline. This site is a web view of the same dataset, and every figure on it is computed from the program's committed seed files. Entered in the Seva First Innovation Challenge 2026, Theme 2, category REG-009-OP: Safety and Disaster Management.",
  },
  notFound: {
    figure: "404",
    title: "No camp at that address.",
    body: "The link is wrong, or the page has moved.",
    action: { label: "Back to the start", href: "/" },
  },
  app: {
    district: "Morigaon district",
    date: "14 July 2026",
    qty: (count: string, unit: string) => `${count} ${unit}`,
    notRationed: "Not rationed",
    dash: "—",
    nav: [
      { label: "Overview", href: "/app" },
      { label: "Camps", href: "/app/camps" },
      { label: "Register", href: "/app/register" },
      { label: "Settings", href: "/app/settings" },
    ],
    status: { low: "Low", watch: "Watch", ok: "OK" },
    dashboard: {
      title: "Overview",
      countLabel: (count: number) => [
        count === 1 ? "item below" : "items below",
        "three days",
        "of supply",
      ],
      daysUnit: (days: string) => (days === "1.0" ? "day of supply" : "days of supply"),
      allCamps: "See all camps",
      transferTitle: "Move stock",
      transferAction: "Apply these moves",
      transferDone: "Applied. This is a demo, so nothing left your browser.",
      transferEmpty: "No camp is below three days of supply.",
      transferLine: (quantity: string, unit: string, item: string) =>
        `${quantity} ${unit} of ${item.toLowerCase()}`,
      transferRoute: (from: string, to: string) => `${from} to ${to}`,
      alert: (symptom: string) => `Outbreak alert: ${symptom.toLowerCase()}`,
    },
    camps: {
      title: "All camps",
      columns: {
        name: "Camp",
        occupancy: "Occupancy",
        shelters: "Shelters",
        supply: "Days of supply",
        status: "Status",
      },
      occupancy: (occupancy: number, capacity: number) => `${occupancy} of ${capacity}`,
      full: "Full",
      empty: {
        title: "No camps yet.",
        body: "Open a camp and its register, stock and health log start counting.",
      },
    },
    detail: {
      back: "All camps",
      daysLeft: "days of supply, at the current rate of issue",
      critical: (item: string) => `Thinnest item: ${item.toLowerCase()}`,
      facts: {
        district: "District",
        occupancy: "Occupancy",
        free: "Beds free",
        shelters: "Shelters",
        number: "Camp number",
      },
      stockTitle: "Stock",
      columns: {
        item: "Item",
        onHand: "On hand",
        perDay: "Issued per day",
        supply: "Days of supply",
        status: "Status",
      },
      healthTitle: "Health log",
      healthLine: (cases: number, threshold: number) =>
        `${cases} in the last three days, alert at ${threshold}`,
      chartCaption: "Days of supply by item, thinnest item filled",
      chartLabel: (parts: string[]) => `Days of supply by item: ${parts.join(", ")}`,
    },
    register: {
      title: "Register a family",
      back: "All camps",
      intro:
        "The ration and the vulnerability score are computed from the ages as you type them, the same way the console program does it.",
      fields: {
        head: { label: "Head of family", hint: "The name the register is kept under" },
        village: { label: "Village", hint: "Where the family came from" },
      },
      members: {
        title: "Members",
        hint: "Age decides the ration and the class: 0 to 2 infant, 3 to 12 child, 13 to 59 adult, 60 and over elderly.",
        name: "Name",
        age: "Age",
        pregnant: "Pregnant",
        disabled: "Disabled",
        lactating: "Breastfeeding",
        duty: "Takes a camp duty",
        add: "Add a member",
        remove: "Remove",
        removeLabel: (position: number) => `Remove member ${position}`,
        position: (position: number) => `Member ${position}`,
      },
      types: {
        infant: "Infant",
        child: "Child",
        adult: "Adult",
        elderly: "Elderly",
        "resident-volunteer": "Resident volunteer",
      },
      card: {
        title: "Ration card",
        members: "Members",
        composition: (counts: string[]) => counts.join(", "),
        daily: "Daily ration",
        score: "Vulnerability",
        scoreNote:
          "Infant 40, elderly 35, child 20, adult 10, plus 25 pregnant, 20 disabled, 15 breastfeeding.",
        dutyNote:
          "A resident who also works a camp duty inherits Person once, through two paths. The class resolves the ambiguity in favour of Adult, so the duty changes the type but not the ration.",
        empty: "Add a member and the card fills in.",
      },
      submit: "Register family",
      errorSummary: "There is a problem",
      errorLine: (label: string, message: string) => `${label}: ${message}`,
      errors: {
        required: "Enter a value",
        age: "Enter an age from 0 to 120",
        noMembers: "Add at least one member",
      },
      saved: {
        title: "Registered.",
        body: "This is a demo, so nothing was stored.",
        again: "Register another",
      },
    },
    settings: {
      title: "Settings",
      fields: {
        district: { label: "District" },
        reportDay: {
          label: "Report day",
          hint: "The day the district report is due",
          options: ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday"],
        },
        threshold: { label: "Call stock critical below", hint: "In days" },
        autosave: { label: "Autosave to the CSV files while the app is open" },
      },
      submit: "Save settings",
      errors: { threshold: "Enter a whole number from 1 to 60" },
      saved: "Saved. This is a demo, so nothing was stored.",
    },
    states: {
      title: "States",
      intro:
        "Every screen has an empty, loading and error version. This page shows all three so they can be restyled in one place.",
      empty: "Empty",
      loading: "Loading",
      error: "Error",
      loadingLabel: "Counting stock",
      errorTitle: "That didn't load.",
      errorBody: "The camp data could not be read. The CSV files on disk are untouched.",
      retry: "Try again",
    },
  },
};

export type Site = typeof site;
